// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Data/Stage/TileData.h"
#include "Lethe/Data/Stage/RoomData.h"
#include "TileManagerSubsystem.generated.h"

class ATile;
class UStageInitData;
struct FStageData;

/**
 * 타일 생성을 책임지는 월드 서브시스템
 */
UCLASS(Config = Game)
class LETHE_API UTileManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface
	
	//초기화
	UFUNCTION(BlueprintCallable)
	void MakeNewTileMap();

	template <typename BFSConditionFunc, typename SelectConditionFunc>
	void TileBFS(const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, TSet<FCubeCoord>& OutCoords, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition);

	/**
	 * StartTile부터 TargetTile까지 거리를 측정에 특화된 함수입니다.
	 * 기존 TileBFS를 활용하기엔 OutCoords가 계산에 불필요하므로 성능을 위해 따로 구현했습니다.
	 */
	int32 GetTileDistance(const ATile* StartTile, const ATile* TargetTile, const EBFSType BFSType);
	
	// StartTile에서 TargetTile까지의 "모든 최단 경로"를 Out 인자로 뱉어주는 함수입니다.
	bool FindShortestPath(const ATile* StartTile, const ATile* TargetTile, TArray<TArray<ATile*>>& OutPathTilesArray);

	UFUNCTION(BlueprintPure)
	int32 GetTileFloor(const ATile* Tile);

	void AddToStandingOrReservedMoveTiles(ATile* Tile);
	void RemoveToStandingOrReservedMoveTiles(ATile* Tile);
	void EmptyStandingOrReservedMoveTiles();

	UFUNCTION(BlueprintPure)
	bool CanMoveToTileForAI(ATile* Tile) const;
	
	ATile* GetTile(const FCubeCoord& InCubeCoord);
	
	UFUNCTION(BlueprintCallable)
	bool MapTileAndActor(ATile* InTile, AActor* InActor);
	void UnmapByTile(ATile* InTile);
	void UnmapByActor(AActor* InActor);

	UFUNCTION(BlueprintCallable)
	AActor* GetActorOnTile(const ATile* InTile) const;
	UFUNCTION(BlueprintCallable)
	ATile* GetTileUnderActor(const AActor* InActor) const;

private:
	const FStageData* GetStageData(const FName& StageName) const;
	
	//맵 데이터 초기화
	void InitMapData(const UStageInitData* StageInitData);
	//높낮이맵 제작 알고리즘
	void MakeFloorData(const FRandomStream* RandomStream, const UStageInitData* StageInitData);
	//타일맵 제작 알고리즘
	void MakeEventData(const FRandomStream* RandomStream, const UStageInitData* StageInitData);
	//타일 생성
	void MakeTileActor(const FStageData* StageData, const UStageInitData* StageInitData);

	//크기 만큼의 좌표 영역을 반환
	void GetCoordFromRange(const FCubeCoord& CenterCoord, TArray<FCubeCoord>& OutCoordList, int32 Width, int32 Height) const;
	//Cube좌표를 World좌표로 전환
	FVector CubeCoordToWorldCoord(const FCubeCoord& Coord) const;
	
private:
	UPROPERTY(Config)
	TSoftObjectPtr<UDataTable> StageDataTable;

	//타일과 타일 사이의 간격
	static constexpr float TileWidthInterval = 173.205f;
	static constexpr float TileHeightInterval = 150.f;

	UPROPERTY()
	TMap<FCubeCoord, FTileData> TileDataMap;
	UPROPERTY()
	TMap<int32, FRoomData> RoomDataMap;

	// 탐색을 위해 양방향으로 타일과 액터(캐릭터)를 매핑하는 Map입니다.
	TMap<TWeakObjectPtr<ATile>, TWeakObjectPtr<AActor>> TileToActorMap;
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<ATile>> ActorToTileMap;

	// Enemy AI가 현재 서있거나, MoveAbility로 이동하기 위해 예약한 타일로, 다른 Enemy AI가 동일한 타일을 선택하지 않도록 막는 역할입니다.
	TSet<TWeakObjectPtr<ATile>> StandingOrReservedMoveTilesForAI;
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
void UTileManagerSubsystem::TileBFS(const FCubeCoord& StartCoord, const int32 MaxDepth, const EBFSType BFSType, TSet<FCubeCoord>& OutCoords, const BFSConditionFunc& BFSCondition, const SelectConditionFunc& SelectCondition)
{
	TSet<FCubeCoord> Visited;
	
	if (!TileDataMap.Contains(StartCoord))
	{
		return;
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

		if (SelectCondition(CurrentCoord, CurrentTileData, CurrentDepth))
		{
			OutCoords.Emplace(CurrentCoord);
		}

		//뻗어 나갈 타일들에 대한 조건 검사
		if (CurrentDepth + 1 > MaxDepth)
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
