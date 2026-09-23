// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "CardContainerTypes.generated.h"

/**
 * 전투 중 카드 한 장의 정체를 나타내는 구조체입니다.
 * 덱에 같은 CardId의 카드가 여러 장 있을 수 있으므로, 전투 내에서 고유한 InstanceId로 구분합니다.
 * AbilitySpecHandle은 재시작 후 무효하기 때문에 모델에서는 사용하지 않고, 뷰(CardActor)에서만 보관합니다.
 */
USTRUCT()
struct FCardInstance
{
	GENERATED_BODY()

	UPROPERTY()
	int32 InstanceId = INDEX_NONE;

	UPROPERTY()
	uint64 CardId = 0;

	UPROPERTY()
	int32 CardLevel = 1;

	/** 소유 캐릭터의 태그입니다. */
	UPROPERTY()
	FGameplayTag CharacterTag;
};

/** 모델이 관리하는 카드의 영역입니다. 뷰의 ECardContainer(Selected 포함)와는 별개입니다. */
UENUM()
enum class ECardZone : uint8
{
	None,
	Deck,
	Hand,
	Graveyard,
};

/**
 * 캐릭터 한 명의 Deck/Hand/Graveyard를 InstanceId로 기록하는 구조체입니다.
 */
USTRUCT()
struct FCharacterCardZones
{
	GENERATED_BODY()

	/** 덱의 카드 목록으로, Last()가 맨 위 카드입니다. */
	UPROPERTY()
	TArray<int32> Deck;

	/**
	 * 드로우한 카드가 배치된 슬롯 배열입니다.
	 * 드로우하며 정렬된 카드들은 사용 시점이 아닌 턴 종료 시에 재정렬되므로, 카드를 사용해도 슬롯은 유지되고 값만 INDEX_NONE이 됩니다.
	 */
	UPROPERTY()
	TArray<int32> HandSlots;

	UPROPERTY()
	TArray<int32> Graveyard;
};

/**
 * 카드 컨테이너의 전체 상태를 순수 데이터로 담는 스냅샷입니다.
 * 저장/로드와 되돌리기 양쪽에서 공용으로 사용하며, 객체 포인터를 포함하지 않습니다.
 */
USTRUCT()
struct FCardContainerSnapshot
{
	GENERATED_BODY()

	/** 캐릭터 순서로, 핸드 슬롯 정렬과 덱 박스 인덱스의 기준입니다. */
	UPROPERTY()
	TArray<FGameplayTag> CharacterOrder;

	UPROPERTY()
	TArray<FCardInstance> Instances;

	UPROPERTY()
	TMap<FGameplayTag, FCharacterCardZones> ZonesByCharacter;

	/** 다음 셔플에 사용될 FRandomStream의 현재 시드입니다. */
	UPROPERTY()
	int32 ShuffleStreamSeed = 0;
};
