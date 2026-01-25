// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Data/Stage/TileData.h"
#include "TileManagerSubsystem.generated.h"

struct FStageData;
class UStageInitData;
class ATile;

UENUM()
enum class EBFSType : uint8
{
	Connection,
	Through,
};

/**
 * 타일 생성을 책임지는 월드 서브시스템
 */
UCLASS(Config = Game)
class LETHE_API UTileManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//초기화
	UFUNCTION(BlueprintCallable)
	void MakeNewTileMap();

	template <typename BFSConditionFunc, typename SelectConditionFunc>
	TSet<FCubeCoord> TileBFS(const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition);

private:
	const FStageData* GetStageData(const FName& StageName) const;
	
	//맵 데이터 초기화
	void InitMapData(const FStageData* StageData, const UStageInitData* StageInitData);
	//높낮이맵 제작 알고리즘
	void MakeFloorData(const FRandomStream* RandomStream, const UStageInitData* StageInitData);
	//타일맵 제작 알고리즘
	void MakeEventData(const FStageData* StageData, const UStageInitData* StageInitData);
	//타일 생성
	void MakeTileActor(const FStageData* StageData, const UStageInitData* StageInitData);

	//크기 만큼의 좌표 영역을 반환
	void GetCoordFromRange(const FCubeCoord& CenterCoord, TArray<FCubeCoord>& OutCoordList, int32 Width, int32 Height) const;
	//Cube좌표를 World좌표로 전환
	void CubeCoordToWorldCoord(const FCubeCoord& Coord, FVector& OutVector) const;
	//배열 랜덤 셔플
	void ShuffleArray(const FRandomStream* RandomStream, TArray<FCubeCoord>& Array) const;
	
private:
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> StageDataTable;
	
	//각 방향으로의 오프셋값, ETileDirection과 조합해서 사용
	FCubeCoord DirectionOffsets[6] =
	{
		FCubeCoord(+0, -1), // LeftTop
		FCubeCoord(-1, +0), // Left
		FCubeCoord(-1, +1), // LeftBottom
		FCubeCoord(+0, +1), // RightBottom
		FCubeCoord(+1, +0), // Right
		FCubeCoord(+1, -1), // RightTop
	};

	//타일과 타일 사이의 간격
	static constexpr float TileWidthInterval = 173.205f;
	static constexpr float TileHeightInterval = 150.f;

	UPROPERTY()
	TMap<FCubeCoord, FTileData> TileDataMap;
};

/**
 * BFSCondition은 BFS를 뻗어나가는 데 필요한 추가 조건 작성
 * SelectCondition은 그 중 검출하고 싶은 타일의 조건 작성
 * BFSType은 BFSCondition과 유사하지만, 매우 자주 사용될 조건들을 간편하게 묶어둠 (꼭 필요한지는 모르겠음)
 * 템플릿 선언 내에서 작성된 검사는 공통 BFS의 검사 내용만
 */
template <typename BFSConditionFunc, typename SelectConditionFunc>
TSet<FCubeCoord> UTileManagerSubsystem::TileBFS(const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition)
{
	TSet<FCubeCoord> Visited;
	TSet<FCubeCoord> Selected;
	
	if (!TileDataMap.Contains(StartCoord))
	{
		return Visited;
	}
	
	TQueue<TPair<FCubeCoord, int32>> Queue;
	constexpr int32 StartDepth = 0;
	FTileData* CurrentTileData = TileDataMap.Find(StartCoord);
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

		if (SelectCondition(CurrentTileData, CurrentDepth))
		{
			Selected.Emplace(CurrentCoord);
		}

		//뻗어 나갈 타일들에 대한 조건 검사
		if (CurrentDepth + 1 > MaxDepth)
		{
			continue;
		}
		
		for (int32 Dir = 0; Dir < 6; ++Dir)
		{
			const FCubeCoord NextCoord = CurrentCoord + DirectionOffsets[Dir];
			
			if (Visited.Contains(NextCoord))
			{
				continue;
			}

			const FTileData* NextTileData = TileDataMap.Find(NextCoord);

			if (!NextTileData)
			{
				continue;
			}
			
			switch (BFSType)
			{
				case EBFSType::Connection:
					if (!CurrentTileData->bConnections[Dir])
					{
						continue;
					}
				break;
				case EBFSType::Through:
				break;
				default:
				break;
			}

			if (!BFSCondition())
			{
				continue;
			}
			
			Queue.Enqueue({NextCoord, CurrentDepth + 1});
		}
	}

	return Selected;
}
