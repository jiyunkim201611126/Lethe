// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CubeCoord.h" 
#include "TileData.h" 
#include "StageData.h"
#include "TileSubsystem.generated.h"

class ATile;

UENUM()
enum class EBFSType : uint8
{
	Connection,
	Through,
};

/** 타일 생성을 책임지는 월드 서브시스템
 * 
 */
UCLASS()
class LETHE_API UTileSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//초기화
	UFUNCTION(BlueprintCallable)
	void MakeNewTileMap();

	template <typename BFSConditionFunc, typename SelectConditionFunc>
	TSet<FCubeCoord> TileBFS(
		const FCubeCoord& StartCoord,
		const int32 MaxDepth,
		const EBFSType BFSType,
		const BFSConditionFunc& BFSCondition,
		const SelectConditionFunc& SelectCondition);

private:
	//각 방향으로의 오프셋값, ETileDirection과 조합해서 사용
	FCubeCoord DirectionOffsets[6] =
	{
		FCubeCoord(0,  -1), // LeftTop
		FCubeCoord(-1, 0), // Left
		FCubeCoord( -1, +1), // LeftBottom
		FCubeCoord(0,  +1), // RightBottom
		FCubeCoord(+1, 0), // Right
		FCubeCoord( 1, -1), // RightTop
	};

	//타일과 타일 사이의 간격
	static constexpr float TileWidthInterval = 173.205f;
	static constexpr float TileHeightInterval = 150.f;

	//맵 데이터 초기화
	void InitMapData(const FStageData* StageData, const UStageInitData* StageInitData);
	//높낮이맵 제작 알고리즘
	void MakeFloorData(const FRandomStream* RandomStream, const UStageInitData* StageInitData);
	//타일맵 제작 알고리즘
	void MakeEventData(const FStageData* StageData, const UStageInitData* StageInitData);
	//타일 생성
	void MakeTileActor(const FStageData* StageData, const UStageInitData* StageInitData);

	//크기 만큼의 좌표 영역을 반환
	TArray<FCubeCoord> GetCoordFromRange(const FCubeCoord& CenterCoord, int32 Width, int32 Height) const;
	//Cube좌표를 World좌표로 전환
	FVector CubeCoordToWorldCoord(const FCubeCoord& Coord);
	//배열 랜덤 셔플
	void ShuffleArray(const FRandomStream* RandomStream, TArray<FCubeCoord>& Array);

	UPROPERTY()
	TMap<FCubeCoord, FTileData> TileDataMap;
	TMap<FCubeCoord, ATile*> TileActorMap;


	
};

//BFSCondition은 BFS를 뻗어나가는 데 필요한 추가 조건 작성
//SelectCondition은 그 중 검출하고 싶은 타일의 조건 작성
//BFSType은 BFSCondition과 유사하지만, 매우 자주 사용될 조건들을 간편하게 묶어둠 (꼭 필요한지는 모르겠음)
//템플릿 선언 내에서 작성된 검사는 공통 BFS의 검사 내용만
template <typename BFSConditionFunc, typename SelectConditionFunc>
TSet<FCubeCoord> UTileSubsystem::TileBFS(
	const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType,const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition)
{
	TSet<FCubeCoord> visited;
	TSet<FCubeCoord> selected;
	
	if (!TileDataMap.Contains(StartCoord))
	{
		return visited;
	}
	
	TQueue<TPair<FCubeCoord, int32>> queue;
	constexpr int32 startDepth = 0;
	FTileData* currentTileData = TileDataMap.Find(StartCoord);
	queue.Enqueue({StartCoord, startDepth});
	
	while (!queue.IsEmpty())
	{
		TPair<FCubeCoord, int32> current;
		queue.Dequeue(current);
		const FCubeCoord& currentCoord = current.Key;
		const int32 currentDepth = current.Value;

		//현재 타일에 대한 조건 검사
		if (visited.Contains(currentCoord))
		{
			continue;
		}

		visited.Add(currentCoord);
		currentTileData = TileDataMap.Find(currentCoord);

		if (SelectCondition(currentTileData, currentDepth))
		{
			selected.Add(currentCoord);
		}

		//뻗어 나갈 타일들에 대한 조건 검사
		if (currentDepth + 1 > MaxDepth)
		{
			continue;
		}
		
		for (int dir = 0; dir < 6; ++dir)
		{
			const FCubeCoord nextCoord = currentCoord + DirectionOffsets[dir];
			
			if (visited.Contains(nextCoord))
			{
				continue;
			}
			
			FTileData* nextTileData = TileDataMap.Find(nextCoord);

			if (!nextTileData)
			{
				continue;
			}
			
			switch (BFSType)
			{
				case EBFSType::Connection:
					if (!currentTileData->bConnections[dir])
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
			
			queue.Enqueue({nextCoord, currentDepth + 1});
		}
	}

	return selected;
}
