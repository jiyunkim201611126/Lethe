// Fill out your copyright notice in the Description page of Project Settings.

#include "TileManagerSubsystem.h"

#include "TileGenerator.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"

void UTileManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UTileManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	TileDataMap.Empty();
}

void UTileManagerSubsystem::MakeNewTileMap()
{
	// 절차적 생성 맵 생성을 시작하는 함수 (GameMode에서 블루프린트 호출)
	if (const FStageData* StageData = GetStageData(FName("Forest")))
	{
		if (const UStageInitData* StageInitData = StageData->StageInitData.LoadSynchronous())
		{
			FTileGenerationResult GenerationResult;
			if (FTileGenerator::GenerateTileMap(GetWorld(), StageData, StageInitData, GenerationResult))
			{
				TileDataMap = MoveTemp(GenerationResult.TileDataMap);
				RoomDataMap = MoveTemp(GenerationResult.RoomDataMap);
			}
		}
	}
}

const FStageData* UTileManagerSubsystem::GetStageData(const FName& StageName) const
{
	if (const UDataTable* LoadedStageDataTable = StageDataTable.LoadSynchronous())
	{
		return LoadedStageDataTable->FindRow<FStageData>(StageName, TEXT(""));
	}
	return nullptr;
}

int32 UTileManagerSubsystem::GetTileDistance(const ATile* StartTile, const ATile* TargetTile, const EBFSType BFSType)
{
	if (!StartTile || !TargetTile)
	{
		return INDEX_NONE;
	}

	if (StartTile == TargetTile)
	{
		return 0;
	}

	const FCubeCoord StartCoord = StartTile->GetCubeCoord();
	const FCubeCoord TargetCoord = TargetTile->GetCubeCoord();
	if (!TileDataMap.Contains(StartCoord) || !TileDataMap.Contains(TargetCoord))
	{
		return INDEX_NONE;
	}

	int32 FoundDepth = INDEX_NONE;
	TSet<FCubeCoord> DummyCoords;

	TileBFS(StartCoord, 999, BFSType, DummyCoords,
		[&FoundDepth](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return FoundDepth == INDEX_NONE;
		},
		[&TargetCoord, &FoundDepth](const FCubeCoord CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
		{
			if (CurrentCoord == TargetCoord)
			{
				FoundDepth = CurrentDepth;
				return true;
			}
			return false;
		});

	// 만약 고립된 타일이 TargetTile로 들어올 경우, 맵 전체를 탐색하며 스파이크가 튀어 프레임 드랍이 발생할 수 있습니다.
	// TODO: 고립된 타일이 생길 수 있는 기획이 들어온다면 로직을 수정해야 합니다.
	return FoundDepth;
}

bool UTileManagerSubsystem::BuildShortestPathSearchData(const ATile* StartTile, const ATile* TargetTile, FShortestPathSearchData& OutSearchData, const bool bIgnoreActor) const
{
	OutSearchData.DistanceMap.Reset();
	OutSearchData.ParentCoordMap.Reset();
	OutSearchData.ShortestDistanceToTarget = INDEX_NONE;

	if (!StartTile || !TargetTile)
	{
		return false;
	}

	const FCubeCoord StartCoord = StartTile->GetCubeCoord();
	const FCubeCoord TargetCoord = TargetTile->GetCubeCoord();

	if (!TileDataMap.Contains(StartCoord) || !TileDataMap.Contains(TargetCoord))
	{
		return false;
	}

	if (StartCoord == TargetCoord)
	{
		return false;
	}

	TQueue<TPair<FCubeCoord, int32>> NextCoordsQueue;

	NextCoordsQueue.Enqueue({ StartCoord, 0 });
	OutSearchData.DistanceMap.Emplace(StartCoord, 0);

	// BFS 탐색을 시작합니다.
	while (!NextCoordsQueue.IsEmpty())
	{
		TPair<FCubeCoord, int32> Current;
		NextCoordsQueue.Dequeue(Current);
		const FCubeCoord CurrentCoord = Current.Key;
		const int32 CurrentDepth = Current.Value;

		const FTileData* CurrentTileData = TileDataMap.Find(CurrentCoord);
		if (!CurrentTileData)
		{
			continue;
		}

		// TargetCoord를 찾은 뒤에는, 최단 거리보다 깊거나 같은 Distance의 타일은 더 확장할 필요가 없으므로 스킵합니다.
		if (OutSearchData.ShortestDistanceToTarget != INDEX_NONE && CurrentDepth >= OutSearchData.ShortestDistanceToTarget)
		{
			continue;
		}
		
		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			// 이동할 타일을 찾는 중이기 때문에 연결된 타일이 아니라면 스킵합니다.
			if (!CurrentTileData->Connections[Direction])
			{
				continue;
			}

			const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(Direction);
			if (!TileDataMap.Contains(NextCoord))
			{
				continue;
			}

			// TargetCoord를 찾은 뒤에는, 최단 거리보다 더 깊은 탐색은 필요 없으므로 스킵합니다.
			const int32 NextDepth = CurrentDepth + 1;
			if (OutSearchData.ShortestDistanceToTarget != INDEX_NONE && NextDepth > OutSearchData.ShortestDistanceToTarget)
			{
				continue;
			}

			// 타일 위에 무언가 있고, 이를 무시하지 않을 예정이라면 스킵합니다.
			if (!bIgnoreActor && NextCoord != TargetCoord && GetActorOnTile(GetTile(NextCoord)))
			{
				continue;
			}

			const int32* ExistingDepth = OutSearchData.DistanceMap.Find(NextCoord);
			if (!ExistingDepth)
			{
				// 처음 도달한 좌표면 깊이를 기록하고 큐에 넣습니다.
				OutSearchData.DistanceMap.Emplace(NextCoord, NextDepth);
				OutSearchData.ParentCoordMap.FindOrAdd(NextCoord).Emplace(CurrentCoord);
				NextCoordsQueue.Enqueue({ NextCoord, NextDepth });
			}
			else if (*ExistingDepth == NextDepth)
			{
				// 이미 같은 최단 거리로 도달 가능한 경우에는, 최단 경로 복원을 위해 부모를 추가로 기록합니다.
				TArray<FCubeCoord>& Parents = OutSearchData.ParentCoordMap.FindOrAdd(NextCoord);
				if (!Parents.Contains(CurrentCoord))
				{
					Parents.Emplace(CurrentCoord);
				}
			}
			// 기존 기록보다 더 짧게 도달하는 경우는 알고리즘상 존재하지 않기 때문에 따로 처리하지 않습니다.

			// TargetCoord를 발견했다면 그 Distance를 기록합니다.
			if (NextCoord == TargetCoord)
			{
				OutSearchData.ShortestDistanceToTarget = NextDepth;
			}
		}
	}

	return OutSearchData.ShortestDistanceToTarget != INDEX_NONE;
}

bool UTileManagerSubsystem::FindShortestPath(const ATile* StartTile, const ATile* TargetTile, TArray<TArray<ATile*>>& OutPathTilesArray, const bool bIgnoreActor) const
{
	OutPathTilesArray.Reset();
	FShortestPathSearchData SearchData;
	if (!BuildShortestPathSearchData(StartTile, TargetTile, SearchData, bIgnoreActor))
	{
		return false;
	}

	const FCubeCoord StartCoord = StartTile->GetCubeCoord();
	const FCubeCoord TargetCoord = TargetTile->GetCubeCoord();
	
	// TargetTile을 처음 발견한 최단 깊이입니다. 아직 못 찾았으면 INDEX_NONE입니다.
	const int32 ShortestDistanceToTarget = SearchData.ShortestDistanceToTarget;

	// TargetTile -> StartTile 방향으로 부모를 따라가며 모든 최단 경로를 생성합니다.
	// CurrentReversedPath는 [Target, ..., Start] 순서로 쌓입니다.
	TArray<FCubeCoord> CurrentReversedPath;
	CurrentReversedPath.Reserve(ShortestDistanceToTarget + 1);

	// 경로 복원에서 같은 좌표를 반복 조회하므로 좌표 -> 타일을 캐시합니다.
	TMap<FCubeCoord, ATile*> TileCache;
	TileCache.Reserve(SearchData.DistanceMap.Num());

	/**
	 * ParendCoordMap을 따라 TargetTile -> StartTile 방향으로 재귀 탐색하며 모든 최단 경로를 복원합니다.
	 * CurrentReversedPath는 현재 분기에서 [Target, ..., Start] 순서로 쌓이는 경로입니다.
	 * StartCoord에 도달하면 StartTile을 제외하고 역순으로 읽어 [Start 다음 타일, ..., Target] 경로를 생성합니다.
	 * 각 분기 탐색이 끝나면 Pop으로 현재 좌표를 제거해 다음 분기를 위한 경로 상태를 복구합니다.
	 */
	TFunction<void(const FCubeCoord&)> BuildAllShortestPaths = [&](const FCubeCoord& CurrentCoord)
	{
		CurrentReversedPath.Emplace(CurrentCoord);

		if (CurrentCoord == StartCoord)
		{
			TArray<ATile*> PathTiles;
			PathTiles.Reserve(CurrentReversedPath.Num() - 1);

			for (int32 Index = CurrentReversedPath.Num() - 2; Index >= 0; --Index)
			{
				const FCubeCoord& PathCoord = CurrentReversedPath[Index];
				ATile** CachedTile = TileCache.Find(PathCoord);
				ATile* PathTile = CachedTile ? *CachedTile : GetTile(PathCoord);
				if (!CachedTile)
				{
					TileCache.Emplace(PathCoord, PathTile);
				}

				if (!PathTile)
				{
					PathTiles.Reset();
					break;
				}
				PathTiles.Emplace(PathTile);
			}

			if (!PathTiles.IsEmpty())
			{
				OutPathTilesArray.Emplace(MoveTemp(PathTiles));
			}

			CurrentReversedPath.Pop();
			return;
		}

		if (const TArray<FCubeCoord>* Parents = SearchData.ParentCoordMap.Find(CurrentCoord))
		{
			for (const FCubeCoord& ParentCoord : *Parents)
			{
				BuildAllShortestPaths(ParentCoord);
			}
		}

		CurrentReversedPath.Pop();
	};

	BuildAllShortestPaths(TargetCoord);

	return !OutPathTilesArray.IsEmpty();
}

bool UTileManagerSubsystem::FindPrioritizedPathTiles(const ATile* StartTile, const ATile* TargetTile, const int32 MoveDistance, TArray<ATile*>& OutPathTiles, const bool bIgnoreActor) const
{
	OutPathTiles.Reset();
	if (MoveDistance <= 0)
	{
		return false;
	}

	FShortestPathSearchData SearchData;
	if (!BuildShortestPathSearchData(StartTile, TargetTile, SearchData, bIgnoreActor))
	{
		return false;
	}

	// 이번 턴에 최대 이동 가능한 거리를 계산합니다.
	const int32 MaxPriorityDistance = FMath::Min(MoveDistance, SearchData.ShortestDistanceToTarget);

	// 거리별 후보 좌표 저장 공간을 준비합니다.
	// Index 1에는 '한 칸 이동 후보', Index 2에는 '두 칸 이동 후보' ...
	TArray<TSet<FCubeCoord>> PrioritizedCoordsByDistance;
	PrioritizedCoordsByDistance.SetNum(MaxPriorityDistance + 1);

	// TargetTile -> StartTile 방향으로 부모를 따라가며 최단 경로를 생성합니다.
	TSet<FCubeCoord> CurrentCoords;
	CurrentCoords.Emplace(TargetTile->GetCubeCoord());

	// 현재 Distance에 있는 좌표 집합에서 부모들을 모아 Distance - 1 좌표 집합을 생성합니다.
	for (int32 Distance = SearchData.ShortestDistanceToTarget; Distance > 0; --Distance)
	{
		TSet<FCubeCoord> NextCoords;
		for (const FCubeCoord& Coord : CurrentCoords)
		{
			if (const TArray<FCubeCoord>* Parents = SearchData.ParentCoordMap.Find(Coord))
			{
				for (const FCubeCoord& ParentCoord : *Parents)
				{
					NextCoords.Emplace(ParentCoord);
				}
			}
		}

		// 이번 턴 후보 거리 안에 있으면 저장합니다.
		const int32 ParentDistance = Distance - 1;
		if (ParentDistance >= 1 && ParentDistance <= MaxPriorityDistance)
		{
			PrioritizedCoordsByDistance[ParentDistance] = NextCoords;
		}

		// 방금 찾은 부모 집합을 기준으로 다시 부모를 찾습니다.
		CurrentCoords = MoveTemp(NextCoords);
	}

	// 만들어진 좌표 집합 가장 먼 이동 후보부터 넣어 우선순위로 정렬합니다.
	for (int32 Distance = MaxPriorityDistance; Distance >= 1; --Distance)
	{
		for (const FCubeCoord& Coord : PrioritizedCoordsByDistance[Distance])
		{
			if (ATile* Tile = GetTile(Coord))
			{
				OutPathTiles.AddUnique(Tile);
			}
		}
	}

	return !OutPathTiles.IsEmpty();
}

void UTileManagerSubsystem::ReservePlayerMoveTile(const AActor* Character, ATile* Tile)
{
	const ATile* CurrentTile = GetTileUnderActor(Character);
	PlayerCharacterReservedTiles.Remove(CurrentTile);
	PlayerCharacterReservedTiles.Emplace(Tile);
}

void UTileManagerSubsystem::RemovePlayerReservedTile(ATile* Tile)
{
	PlayerCharacterReservedTiles.Remove(Tile);
}

void UTileManagerSubsystem::ResetPlayerReservedTile(const TArray<AActor*>& PlayerCharacters)
{
	PlayerCharacterReservedTiles.Reset();
	for (const AActor* PlayerCharacter : PlayerCharacters)
	{
		if (ATile* Tile = GetTileUnderActor(PlayerCharacter))
		{
			PlayerCharacterReservedTiles.Emplace(Tile);
		}
	}
}

bool UTileManagerSubsystem::CanMoveToTileForPlayerCharacter(const ATile* Tile) const
{
	// 예약된 타일을 걸러주지 않으면 빠른 조작 시 플레이어 캐릭터끼리 겹치는 경우가 있으므로 막아줍니다.
	return !PlayerCharacterReservedTiles.Contains(Tile);
}

bool UTileManagerSubsystem::CanMoveToTileForEnemyAI(const ATile* Tile) const
{
	// 타일 위에 아무것도 없다면 이동할 수 있습니다.
	return !GetActorOnTile(Tile);
}

ATile* UTileManagerSubsystem::GetTile(const FCubeCoord& InCubeCoord) const
{
	if (const FTileData* TileData = TileDataMap.Find(InCubeCoord))
	{
		if (TileData->TileActor.IsValid())
		{
			return TileData->TileActor.Get();
		}
	}

	return nullptr;
}

int32 UTileManagerSubsystem::GetTileFloor(const ATile* Tile) const
{
	if (Tile)
	{
		if (const FTileData* TileData = TileDataMap.Find(Tile->GetCubeCoord()))
		{
			return TileData->Floor;
		}
	}
	return INDEX_NONE;
}

bool UTileManagerSubsystem::MapTileAndActor(ATile* InTile, AActor* InActor)
{
	if (!InTile || !InActor)
	{
		return false;
	}

	UnmapByTile(InTile);
	UnmapByActor(InActor);
	
	TileToActorMap.Emplace(InTile, InActor);
	ActorToTileMap.Emplace(InActor, InTile);

	return true;
}

void UTileManagerSubsystem::UnmapByTile(ATile* InTile)
{
	if (!InTile)
	{
		return;
	}
	
	if (const TWeakObjectPtr<AActor>* Actor = TileToActorMap.Find(InTile))
	{
		if (Actor->IsValid())
		{
			ActorToTileMap.Remove(Actor->Get());
		}
	}
	TileToActorMap.Remove(InTile);
}

void UTileManagerSubsystem::UnmapByActor(AActor* InActor)
{
	if (!InActor)
	{
		return;
	}
	
	if (const TWeakObjectPtr<ATile>* Tile = ActorToTileMap.Find(InActor))
	{
		if (Tile->IsValid())
		{
			TileToActorMap.Remove(Tile->Get());
		}
	}
	ActorToTileMap.Remove(InActor);
}

AActor* UTileManagerSubsystem::GetActorOnTile(const ATile* InTile) const
{
	if (TileToActorMap.Contains(InTile))
	{
		const TWeakObjectPtr<AActor> FoundActor = TileToActorMap.FindRef(InTile);
		if (FoundActor.IsValid())
		{
			return FoundActor.Get();
		}
	}

	return nullptr;
}

ATile* UTileManagerSubsystem::GetTileUnderActor(const AActor* InActor) const
{
	if (ActorToTileMap.Contains(InActor))
	{
		const TWeakObjectPtr<ATile> FoundTile = ActorToTileMap.FindRef(InActor);
		if (FoundTile.IsValid())
		{
			return FoundTile.Get();
		}
	}
	return nullptr;
}
