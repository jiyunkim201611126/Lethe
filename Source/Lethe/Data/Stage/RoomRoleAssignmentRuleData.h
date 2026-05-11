// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.h"
#include "TileData.h"
#include "Engine/DataAsset.h"
#include "RoomRoleAssignmentRuleData.generated.h"

UENUM(BlueprintType)
enum class ERoomRole : uint8
{
	None,
	PlayerSpawn,
	Boss,
	StageEnd,
};

UENUM(BlueprintType)
enum class ERoomCoordSlotType : uint8
{
	None,
	Stair,
	PlayerSpawn,
	EnemySpawn,
};

USTRUCT(BlueprintType)
struct FRoomCoordSlot
{
	GENERATED_BODY()

	/**
	 * DataAsset 상으로는 RequiredSpaceRange의 중심을 기준으로 하는 좌표입니다.
	 * RoomManagerSubsystem이 구조체를 생성해 반환하는 경우라면, 월드 좌표입니다.
	 */
	UPROPERTY(EditDefaultsOnly, meta = (HexDirectionButtons))
	FCubeCoord SlotCoord;

	UPROPERTY(EditDefaultsOnly)
	ERoomCoordSlotType SlotType = ERoomCoordSlotType::None;

	/** 할당하면 해당 위치에 스폰합니다. 단, PlayerSpawn 같은 경우 코드상으로 제어하기 때문에 할당해도 무시됩니다. */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> SpawnActorClass;
};

UCLASS()
class LETHE_API URoomRoleAssignmentRuleData : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	ERoomRole RoomRole = ERoomRole::None;

	/** 지정된 Room이 되기 위한 최소 공간을 의미합니다. */
	UPROPERTY(EditDefaultsOnly)
	EBFSType RequiredSpaceRangeBFSType;

	UPROPERTY(EditDefaultsOnly)
	int32 RequiredSpaceRangeDistance;

	/** 해당 공간 중 어느 좌표에 무엇을 스폰할 것인지를 의미합니다. */
	UPROPERTY(EditDefaultsOnly)
	TArray<FRoomCoordSlot> CoordSlots;
};
