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

/**
 * 시야 시스템과 RoomRole을 관장하는 매니저 클래스입니다.
 */
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

	void NotifyCharacterTileChanged(AActor* InCharacter, const ATile* OldTile, ATile* NewTile);

	/** 적 전투 돌입 시 호출되어 해당 타일을 Visible 처리합니다. */
	void RevealEnemyTile(const ATile* InTile);
	
	const FRoomData* GetRoomData(const int32 RoomId) const;
	int32 GetRoomCount() const;

private:
	/** 플레이어 캐릭터 이동 시 호출되어 시야에 필요한 정보들을 갱신합니다. */
	void UpdatePlayerRoomState(const ATile* OldTile, const ATile* NewTile);

	/** 플레이어 캐릭터 이동 시 호출되어 시야를 갱신합니다. */
	void RefreshPlayerVision();

	/** 조건에 맞는 좌표들을 수집해옵니다. */
	void CollectPlayerVisibleCoords(TSet<FCubeCoord>& OutCoords) const;
	void CollectRoomCoords(const FRoomData& RoomData, TSet<FCubeCoord>& OutCoords) const;
	void CollectRoomBorderCoords(const FRoomData& RoomData, TSet<FCubeCoord>& OutRoomCoords, TSet<FCubeCoord>& OutBorderCoords) const;

	/** 기록된 정보들을 토대로 시야를 갱신합니다. */
	void ApplyVisionSnapshot();
	void ApplyVisionSnapshot(const TSet<FCubeCoord>& PlayerVisibleCoords);

	/** Tile 관련 함수지만, 시야 판정에 무게를 두는 함수기 때문에 RoomManager에서 구현합니다. */
	bool IsTileVisibleByPlayer(const ATile* InTile) const;

	FRoomData* GetMutableRoomData(const int32 RoomId);

private:
	UPROPERTY()
	TMap<int32, FRoomData> RoomDataMap;

	TSet<int32> RoleAssignedRoomIds;

	/**
	 * TileManagerSubsystem과는 별개로 RoomManagerSubsystem이 플레이어 캐릭터와 타일을 매핑합니다.
	 * TileManagerSubsystem은 GA_Move 사용 시 즉시 '로직적' 매핑을 갱신, 캐릭터와 목적지 타일을 매핑합니다.
	 * RoomManagerSubsystem은 캐릭터가 목적지까지 선형적으로 이동하며 갱신되는 시야 로직 목적의 '뷰적' 매핑을 갖습니다.
	 */
	TMap<TWeakObjectPtr<AActor>, TWeakObjectPtr<ATile>> PlayerToTile;

	/** 한 번 이상 드러난 타일의 좌표들로, Visible인 좌표들을 포함합니다. */
	TSet<FCubeCoord> RecognizableCoords;

	/** 처음 방문해 일시적으로 볼 수 있는 RoomId입니다. */
	TSet<int32> TemporarilyVisibleRoomIds;
	TMap<int32, FTimerHandle> RoomRevealTimerHandles;
	float RoomTemporarilyVisibleDuration = 3.f;

	/** 전투에 참가해 현재 위치가 강제로 Visible 처리되는 적 좌표들입니다. */
	TSet<FCubeCoord> EnemyVisibleCoords;

	/** 직전 시야 상태를 기록합니다. */
	TMap<FCubeCoord, ETileVisionState> PreviousVisionStates;
};

/**
 * 문서용 페이지가 생기면 아래 문구는 해당 페이지로 이주할 것.
 * 
 * 시야 관련 로직
 *
 * 타일은 무조건 Hidden 상태로 생성되고, 이후 조작으로 인해 Visible 혹은 Recognizable로 변경, 다시 Hidden으로 돌아가지 않습니다.
 * Visible과 Recognizable로 왕복하며 해당 타일 위에 올라가있는 ITileVisionAffectedInterface를 상속받은 Actor의 렌더링 상태도 함께 변경됩니다.
 * 
 * 1. 캐릭터의 View상 타일 변경, Enemy 전투 참가, Room 입장 등의 다양한 이유로 인해 Visible 혹은 Recognizable이 될 조건 판단을 위한 데이터를 기록합니다.
 * 2. 한 번 드러난 좌표는 RecognizableCoords에 무조건 기록합니다.
 * 3. 기록을 마치면 ApplyVisionSnapshot을 호출하고 시야 갱신을 시작합니다.
 * 4. Visible을 먼저 수집하고, Recognizable로 기록되어 있는 좌표 중 Visible이 아닌 좌표만 추가합니다.
 * 5. 이를 PreviousVisionStates와 비교해 변경된 좌표만 시야를 갱신합니다.
 * 6. 갱신된 시야를 PreviousVisionStates에 기록합니다.
 */
