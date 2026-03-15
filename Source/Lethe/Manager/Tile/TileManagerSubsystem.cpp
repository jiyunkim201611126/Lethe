// Fill out your copyright notice in the Description page of Project Settings.

#include "TileManagerSubsystem.h"

#include "TileGenerator.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/StageData.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"

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
	//절차적 생성 맵 생성을 시작하는 함수 (GameMode에서 블루프린트 호출)
	//1. 맵 데이터를 초기화한다.
	//2. 타일에 층고를 만들기 위한 알고리즘을 실행한다.
	//3. 타일에 이벤트를 생성하기 위한 알고리즘을 실행한다.
	//4. 데이터를 기반으로 실제 액터를 생성한다.
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

	TQueue<TPair<FCubeCoord, int32>> NextCoordQueue;
	TSet<FCubeCoord> Visited;

	NextCoordQueue.Enqueue({ StartCoord, 0 });
	Visited.Emplace(StartCoord);

	while (!NextCoordQueue.IsEmpty())
	{
		TPair<FCubeCoord, int32> Current;
		NextCoordQueue.Dequeue(Current);

		const FCubeCoord& CurrentCoord = Current.Key;
		const int32 CurrentDepth = Current.Value;

		const FTileData* CurrentTileData = TileDataMap.Find(CurrentCoord);
		if (!CurrentTileData)
		{
			continue;
		}

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			if (BFSType == EBFSType::Connection && !CurrentTileData->Connections[Direction])
			{
				continue;
			}

			const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(Direction);
			if (Visited.Contains(NextCoord))
			{
				continue;
			}

			if (!TileDataMap.Contains(NextCoord))
			{
				continue;
			}

			const int32 NextDepth = CurrentDepth + 1;
			if (NextCoord == TargetCoord)
			{
				return NextDepth;
			}

			Visited.Emplace(NextCoord);
			NextCoordQueue.Enqueue({ NextCoord, NextDepth });
		}
	}

	// 도달 불가능한 경우 여기로 내려옵니다.
	return INDEX_NONE;
}

bool UTileManagerSubsystem::FindShortestPath(const ATile* StartTile, const ATile* TargetTile, TArray<TArray<ATile*>>& OutPathTilesArray)
{
	OutPathTilesArray.Reset();
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
	// 각 좌표의 "최초 도달 깊이"를 저장합니다. (동일 깊이 중복 부모 기록에 사용)
	TMap<FCubeCoord, int32> DepthMap;
	// 최단 경로 복원을 위해 "이 좌표로 올 수 있는 부모 좌표들"을 저장합니다.
	TMap<FCubeCoord, TArray<FCubeCoord>> ParentCoordMap;

	NextCoordsQueue.Enqueue({StartCoord, 0});
	DepthMap.Emplace(StartCoord, 0);
	// TargetTile을 처음 발견한 최단 깊이입니다. 아직 못 찾았으면 INDEX_NONE입니다.
	int32 ShortestDistanceToTarget = INDEX_NONE;

	// BFS로 탐색하면서 TargetTile이 처음 발견된 깊이까지만 확장합니다.
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

		// TargetTile을 찾은 뒤에는 그 깊이까지만 확장합니다.
		if (ShortestDistanceToTarget != INDEX_NONE && CurrentDepth >= ShortestDistanceToTarget)
		{
			continue;
		}

		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			if (!CurrentTileData->Connections[Direction])
			{
				continue;
			}

			const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(Direction);
			if (!TileDataMap.Contains(NextCoord))
			{
				continue;
			}

			const int32 NextDepth = CurrentDepth + 1;
			if (ShortestDistanceToTarget != INDEX_NONE && NextDepth > ShortestDistanceToTarget)
			{
				continue;
			}

			// Enemy AI 전용 함수기 때문에, 다른 Enemy AI가 점유한 타일이면 스킵합니다.
			// TODO: 추후 공용으로 변경하거나 다른 의도(그냥 무시하고 최단 경로 확인하기 등)로 확장하게 되면 매개변수로 이 조건을 사용할지 말지 결정하는 등의 방식이 필요합니다.
			if (StandingOrReservedMoveTilesForEnemyAI.Contains(GetTile(NextCoord)))
			{
				continue;
			}

			const int32* ExistingDepth = DepthMap.Find(NextCoord);
			if (!ExistingDepth)
			{
				// 처음 도달한 좌표면 깊이를 기록하고 큐에 넣습니다.
				DepthMap.Emplace(NextCoord, NextDepth);
				ParentCoordMap.FindOrAdd(NextCoord).Emplace(CurrentCoord);
				NextCoordsQueue.Enqueue({ NextCoord, NextDepth });
			}
			else if (*ExistingDepth == NextDepth)
			{
				// 이미 같은 깊이로 도달 가능한 경우에는 부모만 추가해 "모든 최단 경로"를 보존합니다.
				TArray<FCubeCoord>& Parents = ParentCoordMap.FindOrAdd(NextCoord);
				if (!Parents.Contains(CurrentCoord))
				{
					Parents.Emplace(CurrentCoord);
				}
			}

			// TargetTile 최초 발견 시 해당 깊이를 최단 깊이로 확정합니다.
			if (NextCoord == TargetCoord)
			{
				ShortestDistanceToTarget = NextDepth;
			}
		}
	}

	if (ShortestDistanceToTarget == INDEX_NONE)
	{
		return false;
	}

	// TargetTile -> StartTile 방향으로 부모를 따라가며 모든 최단 경로를 복원합니다.
	// CurrentReversedPath는 [Target, ..., Start] 순서로 쌓입니다.
	TArray<FCubeCoord> CurrentReversedPath;
	CurrentReversedPath.Reserve(ShortestDistanceToTarget + 1);

	// 경로 복원에서 같은 좌표를 반복 조회하므로 좌표 -> 타일을 캐시합니다.
	TMap<FCubeCoord, ATile*> TileCache;
	TileCache.Reserve(DepthMap.Num());

	TFunction<void(const FCubeCoord&)> BuildAllShortestPaths = [&](const FCubeCoord& CurrentCoord)
	{
		CurrentReversedPath.Emplace(CurrentCoord);

		if (CurrentCoord == StartCoord)
		{
			TArray<ATile*> PathTiles;
			PathTiles.Reserve(CurrentReversedPath.Num() - 1);

			// [Target, ..., Start]를 뒤에서 앞으로 읽으면 [Start 다음 타일, ..., Target]이 됩니다.
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

		if (const TArray<FCubeCoord>* Parents = ParentCoordMap.Find(CurrentCoord))
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

void UTileManagerSubsystem::AddToStandingOrReservedMoveTiles(ATile* Tile)
{
	StandingOrReservedMoveTilesForEnemyAI.Emplace(Tile);
}

void UTileManagerSubsystem::RemoveToStandingOrReservedMoveTiles(ATile* Tile)
{
	StandingOrReservedMoveTilesForEnemyAI.Remove(Tile);
}

void UTileManagerSubsystem::EmptyStandingOrReservedMoveTiles()
{
	StandingOrReservedMoveTilesForEnemyAI.Empty();
}

bool UTileManagerSubsystem::CanMoveToTileForAI(ATile* Tile) const
{
	const bool bIsReserved = StandingOrReservedMoveTilesForEnemyAI.Contains(Tile);
	bool bIsPlayerCharacter = false;
	if (const AActor* ActorOnTile = GetActorOnTile(Tile))
	{
		bIsPlayerCharacter = ActorOnTile->Implements<UPlayableCharacterInterface>();
	}
	return !bIsReserved && !bIsPlayerCharacter;
}

ATile* UTileManagerSubsystem::GetTile(const FCubeCoord& InCubeCoord)
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
