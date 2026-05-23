// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Data/Stage/TileData.h"
#include "TileManagerSubsystem.generated.h"

enum class EStageType : uint8;
enum class ETeamSide : uint8;
class AEnemyCharacterBase;
class ATile;
class UStageInitData;
struct FStageData;

/**
 * 타일 생성 및 타일과 관련된 정보 조회를 담당하는 매니저 클래스입니다.
 */
UCLASS(Config = Game)
class LETHE_API UTileManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface

	void Clear();
	
	/** 타일 생성 시동 함수입니다. */
	UFUNCTION(BlueprintCallable)
	void MakeNewTileMap(const EStageType StageType);

	template <typename BFSConditionFunc, typename SelectConditionFunc>
	void TileBFS(const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, TSet<FCubeCoord>& OutCoords, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition) const;

	UFUNCTION(BlueprintPure)
	int32 GetTileDistance(const ATile* StartTile, const ATile* TargetTile, const EBFSType BFSType) const;
	
	/**
	 * StartTile에서 TargetTile까지의 '모든 최단 경로'를 Out 인자로 뱉어주는 함수입니다.
	 * 생성된 ShortestPathSearchData를 기반으로 모든 경로를 복원하기 때문에, 한 번에 여러 번 호출하면 프레임드랍을 유발할 수 있습니다.
	 */
	bool FindShortestPath(const ATile* StartTile, const ATile* TargetTile, TArray<TArray<ATile*>>& OutPathTilesArray, const bool bIgnoreActor) const;

	/** TargetTile로 도달 가능한 최단 경로 중 대표 경로를 하나만 반환합니다. */
	bool FindPrioritizedPathTiles(const ATile* StartTile, const ATile* TargetTile, const int32 MoveRange, TArray<ATile*>& OutPathTiles, const bool bIgnoreActor) const;

	/** CenterTile을 중심으로 Range 범위 안에 있는 모든 Tile을 OutTiles로 반환합니다. BFSRange를 무시하고 싶을 때 사용합니다. */
	void GetAroundTiles(const ATile* CenterTile, const int32 Range, TArray<ATile*>& OutTiles) const;
	
	bool CanPlayerMoveToTile(const ATile* Tile) const;
	
	UFUNCTION(BlueprintPure)
	bool CanEnemyAIMoveToTile(const ATile* Tile) const;
	
	ATile* GetTile(const FCubeCoord& InCubeCoord) const;
	const TArray<TWeakObjectPtr<ATile>>* GetUnderTiles(const FCubeCoord& InCubeCoord) const;

	UFUNCTION(BlueprintPure)
	int32 GetTileFloor(const ATile* Tile) const;
	
	UFUNCTION(BlueprintCallable)
	void MapTileAndActor(ATile* InTile, AActor* InActor);
	void UnmapByTile(ATile* InTile);
	void UnmapByActor(AActor* InActor);

	UFUNCTION(BlueprintCallable)
	AActor* GetActorOnTile(const ATile* InTile) const;
	UFUNCTION(BlueprintCallable)
	ATile* GetTileUnderActor(const AActor* InActor) const;
	
	const FStageData* GetStageData(const EStageType StageType) const;

private:
	struct FShortestPathSearchData
	{
		/** 각 좌표들에 대한 StartTile로부터의 최소 거리를 기록한 TMap입니다. */
		TMap<FCubeCoord, int32> DistanceMap;
		/** Key 좌표까지 최단 거리로 도달한다고 가정했을 때, 그 직전 좌표들입니다. 경로가 여러 개 나올 수 있으므로 Value는 배열입니다. */
		TMap<FCubeCoord, TArray<FCubeCoord>> ParentCoordMap;
		int32 ShortestDistanceToTarget = INDEX_NONE;
	};

	/** StartTile에서 TargetTile까지의 최단 거리와 부모 DAG를 생성합니다. */
	bool BuildShortestPathSearchData(const ATile* StartTile, const ATile* TargetTile, FShortestPathSearchData& OutSearchData, const bool bIgnoreActor) const;
	
private:
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> StageDataTable;

	UPROPERTY()
	TMap<FCubeCoord, FTileData> TileDataMap;

	/** 탐색을 위해 양방향으로 타일과 액터(캐릭터)를 매핑하는 Map입니다. */
	TMap<TWeakObjectPtr<ATile>, TWeakObjectPtr<AActor>> TileToActorMap;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<ATile>> ActorToTileMap;
};

/**
 * BFSType은 BFSCondition과 유사하지만, 매우 자주 사용될 조건들을 간편하게 묶어둠 (꼭 필요한지는 모르겠음)
 * 템플릿 선언 내에서 작성된 검사는 공통 BFS의 검사 내용만
 * 
 * @param StartCoord BFS 시작 타일 좌표
 * @param MaxDepth 최대 타일간 거리
 * @param BFSType 타일 연결 상태 조건
 * @param OutCoords 최종 검출된 타일 좌표들
 * @param BFSCondition BFS를 뻗어나가는 데 필요한 추가 조건 작성
 * @param SelectCondition 그 중 OutCoords에 Emplace하고 싶은 타일의 조건 작성
 */
template <typename BFSConditionFunc, typename SelectConditionFunc>
void UTileManagerSubsystem::TileBFS(const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, TSet<FCubeCoord>& OutCoords, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition) const
{
	TSet<FCubeCoord> Visited;
	
	if (!TileDataMap.Contains(StartCoord))
	{
		return;
	}
	
	TQueue<TPair<FCubeCoord, int32>> Queue;
	constexpr int32 StartDepth = 0;
	const FTileData* CurrentTileData = TileDataMap.Find(StartCoord);
	Queue.Enqueue({StartCoord, StartDepth});
	
	while (!Queue.IsEmpty())
	{
		TPair<FCubeCoord, int32> Current;
		Queue.Dequeue(Current);
		const FCubeCoord& CurrentCoord = Current.Key;
		const int32 CurrentDepth = Current.Value;

		//현재 타일에 대한 조건 검사
		if (Visited.Contains(CurrentCoord))
		{
			continue;
		}

		Visited.Emplace(CurrentCoord);
		CurrentTileData = TileDataMap.Find(CurrentCoord);

		if (SelectCondition(CurrentCoord, CurrentTileData, CurrentDepth))
		{
			OutCoords.Emplace(CurrentCoord);
		}

		//뻗어 나갈 타일들에 대한 조건 검사
		if (MaxDepth < CurrentDepth + 1)
		{
			continue;
		}
		
		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			const FCubeCoord NextCoord = CurrentCoord + FCubeCoord::GetDirection(Direction);
			if (Visited.Contains(NextCoord))
			{
				continue;
			}

			const FTileData* NextTileData = TileDataMap.Find(NextCoord);
			if (!NextTileData)
			{
				continue;
			}
			
			// TODO: 탐색하지 않는다는 조건에 있어 필요한 매개변수들 여기서 넣어줘야 할 듯? 현재는 단순 true, false로만 해당 타일에서 더 BFS할지 말지 결정하는 중
			if (!BFSCondition(CurrentTileData, NextTileData))
			{
				continue;
			}
			
			switch (BFSType)
			{
				case EBFSType::Connection:
					if (!CurrentTileData->Connections[Direction])
					{
						continue;
					}
				break;
				case EBFSType::Through:
				break;
				default:
				break;
			}
			
			Queue.Enqueue({NextCoord, CurrentDepth + 1});
		}
	}
}
