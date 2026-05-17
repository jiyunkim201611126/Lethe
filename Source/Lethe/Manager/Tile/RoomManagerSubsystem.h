// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/RoomRoleAssignmentRuleData.h"
#include "Lethe/Data/Stage/RoomData.h"
#include "Subsystems/WorldSubsystem.h"
#include "RoomManagerSubsystem.generated.h"

enum class ETileVisionState : uint8;

struct FRoomRolePlacementCandidate
{
	int32 RoomId = INDEX_NONE;
	int32 RoomSize = 0;
	FCubeCoord CenterCoord;
	TArray<FRoomCoordSlot> CoordSlots;
};

UCLASS()
class LETHE_API URoomManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface

	void Clear();
	
	void SetRoomData(TMap<int32, FRoomData>&& InRoomData);

	void NotifyActorTileChanged(const AActor* InActor, const ATile* OldTile, const ATile* NewTile);

	void RevealEnemyTile(const ATile* InTile) const;
	void UpdateEnemyMoveVision(const ATile* OldTile, const ATile* NewTile) const;

	/**
	 * RoomRoleAssignmentRule에 의해 특정 역할을 가진 Room이 될 수 있는 후보군을 수집하는 함수입니다.
	 *
	 * @param RoomRoleAssignmentRuleData 역할을 가진 Room이 될 수 있는 조건을 정의하는 Rule DataAsset입니다.
	 * @param OutCandidates 역할 부여를 위한 Room 후보와 Slot 좌표가 할당될 배열입니다.
	 * @return 성공적으로 수집했다면 true를 반환합니다.
	 */
	bool TryFindRoomRoleCandidates(const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData, TArray<FRoomRolePlacementCandidate>& OutCandidates) const;

	/**
	 * 아직 Role이 부여되지 않은 Room 중에서, StartRoomId 기준 거리순으로 오름차순 정렬된 RoomId들을 반환합니다.
	 * 여기서 거리란, Tile을 기준으로 잡지 않고 Room 자체를 한 칸으로 생각합니다.
	 */
	bool TryGetDistantRoomIds(const int32 StartRoomId, TArray<int32>& OutRoomIds) const;

	void MarkRoomRoleAssigned(const FRoomRolePlacementCandidate& Candidate);
	
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
