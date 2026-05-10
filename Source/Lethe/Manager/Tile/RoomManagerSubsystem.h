// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/RoomData.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManagerSubsystem.generated.h"

enum class ETileVisionState : uint8;
class URoomRoleAssignmentRuleData;
struct FRoomCoordSlot;

UCLASS()
class LETHE_API URoomManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface
	
	void SetRoomData(TMap<int32, FRoomData>&& InRoomData);

	void NotifyActorTileChanged(const AActor* InActor, const ATile* OldTile, const ATile* NewTile);

	void RevealEnemyTile(const ATile* InTile) const;
	void UpdateEnemyMoveVision(const ATile* OldTile, const ATile* NewTile) const;

	bool TryAssignRoomRole(const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData, TArray<TArray<FRoomCoordSlot>>& OutCoordSlotArrays);
	
	const FRoomData* GetRoomData(const int32 RoomId) const;
	int32 GetRoomCount() const;

private:
	void UpdatePlayerRoomState(const ATile* OldTile, const ATile* NewTile);
	void SetRoomVisionState(const int32 InRoomId, FRoomData* RoomData, const ETileVisionState VisionState) const;
	void SetTileStackVisionState(const ATile* InTile, const ETileVisionState VisionState) const;

	// Tile 관련 함수지만, 시야 판정에 무게를 두는 함수기 때문에 RoomManager에서 구현합니다.
	bool IsTileVisibleByPlayer(const ATile* InTile) const;

	FRoomData* GetMutableRoomData(const int32 RoomId);

private:
	UPROPERTY()
	TMap<int32, FRoomData> RoomDataMap;

	TSet<int32> RoleAssignedRoomIds;
};
