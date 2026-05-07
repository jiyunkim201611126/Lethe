// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileManagerSubsystem.h"

#include "RoomManagerSubsystem.h"
#include "TileGenerator.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"
#include "Lethe/Interface/CombatInterface.h"

void UTileManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	TileDataMap.Empty();
}

void UTileManagerSubsystem::MakeNewTileMap()
{
	// 절차적 생성 맵 생성을 시작하는 함수 (GameMode에서 블루프린트 호출)
	if (const FStageData* StageData = GetStageData(EStageType::Forest))
	{
		if (const UStageInitData* StageInitData = StageData->StageInitData.LoadSynchronous())
		{
			FTileGenerationResult GenerationResult;
			if (FTileGenerator::GenerateTileMap(GetWorld(), StageData, StageInitData, GenerationResult))
			{
				TileDataMap = MoveTemp(GenerationResult.TileDataMap);
				if (URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>())
				{
					RoomManagerSubsystem->SetRoomData(MoveTemp(GenerationResult.RoomDataMap));
				}
			}
		}
	}
}

const FStageData* UTileManagerSubsystem::GetStageData(const EStageType StageType) const
{
	// 현재는 Stage가 많지 않아, 테이블 내 모든 데이터를 순회하는 방식입니다.
	// 추후 Stage가 2자릿수로 늘어난다면 Init 타이밍에 TMap으로 캐싱하는 방식으로 교체합니다.
	if (const UDataTable* LoadedStageDataTable = StageDataTable.LoadSynchronous())
	{
		TArray<FStageData*> LoadedStageData;
		LoadedStageDataTable->GetAllRows(TEXT(""), LoadedStageData);
		for (const FStageData* StageData : LoadedStageData)
		{
			if (StageData->StageType == StageType)
			{
				return StageData;
			}
		}
	}
	return nullptr;
}

int32 UTileManagerSubsystem::GetTileDistance(const ATile* StartTile, const ATile* TargetTile, const EBFSType BFSType) const
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
		[&TargetCoord, &FoundDepth](const FCubeCoord& CurrentCoord, const FTileData* CurrentTileData, const int32 CurrentDepth)
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
	OutSearchData.DistanceMap.Add(StartCoord, 0);

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

			/**
			 * 타일 위에 무언가 있다면 매개변수 설정 여부와 액터 종류에 따라 진행합니다.
			 * 비어 있는 중간 타일: 통과
			 * 중간 타일에 같은 팀이 있음: 통과
			 * 중간 타일에 다른 팀이 있음: 막힘
			 * TargetTile에 뭐가 있음: 막힘
			 */
			if (!bIgnoreActor)
			{
				if (AActor* ActorOnNextTile = GetActorOnTile(GetTile(NextCoord)))
				{
					if (NextCoord == TargetCoord)
					{
						continue;
					}

					const ICombatInterface* NextCombatInterface = Cast<ICombatInterface>(ActorOnNextTile);
					const ICombatInterface* SourceCombatInterface = Cast<ICombatInterface>(GetActorOnTile(StartTile));
					if (!NextCombatInterface || !SourceCombatInterface || NextCombatInterface->GetTeamSide() != SourceCombatInterface->GetTeamSide())
					{
						continue;
					}
				}
			}

			const int32* ExistingDepth = OutSearchData.DistanceMap.Find(NextCoord);
			if (!ExistingDepth)
			{
				// 처음 도달한 좌표면 깊이를 기록하고 큐에 넣습니다.
				OutSearchData.DistanceMap.Add(NextCoord, NextDepth);
				OutSearchData.ParentCoordMap.FindOrAdd(NextCoord).Add(CurrentCoord);
				NextCoordsQueue.Enqueue({ NextCoord, NextDepth });
			}
			else if (*ExistingDepth == NextDepth)
			{
				// 최단 거리로 도달하는 경우라면 부모로 추가합니다.
				TArray<FCubeCoord>& Parents = OutSearchData.ParentCoordMap.FindOrAdd(NextCoord);
				if (!Parents.Contains(CurrentCoord))
				{
					Parents.Add(CurrentCoord);
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
		CurrentReversedPath.Add(CurrentCoord);

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
					TileCache.Add(PathCoord, PathTile);
				}

				if (!PathTile)
				{
					PathTiles.Reset();
					break;
				}
				PathTiles.Add(PathTile);
			}

			if (!PathTiles.IsEmpty())
			{
				OutPathTilesArray.Add(MoveTemp(PathTiles));
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

	// TargetTile -> StartTile 방향으로 부모를 따라가며, StartTile 기준 Distance별 후보를 모읍니다.
	TSet<FCubeCoord> CurrentCoords;
	CurrentCoords.Add(TargetTile->GetCubeCoord());

	// CurrentCoords는 현재 Distance에 있는 좌표 집합입니다.
	for (int32 Distance = SearchData.ShortestDistanceToTarget; Distance > 0; --Distance)
	{
		if (Distance <= MaxPriorityDistance)
		{
			for (const FCubeCoord& Coord : CurrentCoords)
			{
				PrioritizedCoordsByDistance[Distance].Add(Coord);
			}
		}

		TSet<FCubeCoord> NextCoords;
		for (const FCubeCoord& Coord : CurrentCoords)
		{
			if (const TArray<FCubeCoord>* Parents = SearchData.ParentCoordMap.Find(Coord))
			{
				for (const FCubeCoord& ParentCoord : *Parents)
				{
					NextCoords.Add(ParentCoord);
				}
			}
		}

		CurrentCoords = MoveTemp(NextCoords);
	}

	// 가장 멀리 갈 수 있는 후보부터 검사합니다.
	for (int32 Distance = MaxPriorityDistance; Distance >= 1; --Distance)
	{
		// 해당 거리의 후보 좌표들을 순회합니다.
		for (const FCubeCoord& CandidateCoord : PrioritizedCoordsByDistance[Distance])
		{
			bool bPathBuilt = true;
			TArray<FCubeCoord> ReversedPath;
			FCubeCoord CurrentCoord = CandidateCoord;

			// 후보 좌표에서 StartTile에 도달할 때까지 부모를 따라갑니다.
			while (CurrentCoord != StartTile->GetCubeCoord())
			{
				ReversedPath.Add(CurrentCoord);

				// 현재 좌표의 부모를 찾고, 없으면 경로 생성을 취소합니다.
				const TArray<FCubeCoord>* Parents = SearchData.ParentCoordMap.Find(CurrentCoord);
				if (!Parents || Parents->IsEmpty())
				{
					bPathBuilt = false;
					break;
				}
				
				// 부모가 여러 개일 순 있지만, 대표 경로 하나만 필요하므로 첫 번째 부모를 선택합니다.
				CurrentCoord = (*Parents)[0];
			}

			// 이 후보에서 경로를 만들지 못 했다면 다음 후보를 봅니다.
			if (!bPathBuilt)
			{
				continue;
			}

			// 역순으로 쌓인 좌표들을 다시 뒤집습니다.
			OutPathTiles.Reset();
			for (int32 Index = ReversedPath.Num() - 1; Index >= 0; --Index)
			{
				if (ATile* Tile = GetTile(ReversedPath[Index]))
				{
					OutPathTiles.Add(Tile);
				}
				else
				{
					OutPathTiles.Reset();
					bPathBuilt = false;
					break;
				}
			}

			// 성공적으로 경로를 생성한 경우 true를 반환합니다.
			if (bPathBuilt && !OutPathTiles.IsEmpty())
			{
				return true;
			}
		}
	}
	return false;
}

void UTileManagerSubsystem::GetAroundTiles(const ATile* CenterTile, const int32 Range, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();

	const FCubeCoord& CenterCoord = CenterTile->GetCubeCoord();

	for (int32 LocalQ = -Range; LocalQ <= Range; ++LocalQ)
	{
		const int32 MinLocalR = FMath::Max(-Range, -LocalQ - Range);
		const int32 MaxLocalR = FMath::Min(Range, -LocalQ + Range);

		for (int32 LocalR = MinLocalR; LocalR <= MaxLocalR; ++LocalR)
		{
			const int32 LocalS = -LocalQ - LocalR;

			const FCubeCoord TileCoord(CenterCoord.Q + LocalQ, CenterCoord.R + LocalR, CenterCoord.S + LocalS);
			if (ATile* Tile = GetTile(TileCoord))
			{
				OutTiles.Add(Tile);
			}
		}
	}
}

bool UTileManagerSubsystem::CanPlayerMoveToTile(const ATile* Tile) const
{
	if (Tile == nullptr)
	{
		return false;
	}
	
	// 타일 무언가 있다면 이동할 수 없습니다.
	return !GetActorOnTile(Tile);
}

bool UTileManagerSubsystem::CanEnemyAIMoveToTile(const ATile* Tile) const
{
	if (Tile == nullptr)
	{
		return false;
	}
	
	// 타일 무언가 있다면 이동할 수 없습니다.
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

void UTileManagerSubsystem::MapTileAndActor(ATile* InTile, AActor* InActor)
{
	if (!InTile || !InActor)
	{
		return;
	}

	const ATile* OldTile = nullptr;
	if (const auto* OldTileWeakPtr = ActorToTileMap.Find(InActor))
	{
		OldTile = OldTileWeakPtr->Get();
	}

	UnmapByTile(InTile);
	UnmapByActor(InActor);
	
	TileToActorMap.Add(InTile, InActor);
	ActorToTileMap.Add(InActor, InTile);

	if (URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>())
	{
		RoomManagerSubsystem->NotifyActorTileChanged(InActor, OldTile, InTile);
	}
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
