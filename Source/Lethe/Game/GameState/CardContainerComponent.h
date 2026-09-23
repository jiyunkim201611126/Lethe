// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lethe/Data/Card/CardContainerTypes.h"
#include "Lethe/Data/TurnPhaseState.h"
#include "CardContainerComponent.generated.h"

struct FSavedCharacterDeck;

DECLARE_MULTICAST_DELEGATE(FOnCardContainerChanged);
DECLARE_MULTICAST_DELEGATE(FOnHandFull);

/**
 * 전투 중 카드의 Deck/Hand/Graveyard 상태를 관리하는 모델 컴포넌트입니다.
 *
 * 카드 액터나 위젯 등 뷰에 대해 전혀 알지 못하며, 상태 변경 시 OnContainerChanged를 통해 뷰에게 알립니다.
 * 상태는 전부 순수 데이터(InstanceId)로 구성되어 있어 Capture/Restore를 통해 저장 및 되돌리기에 사용할 수 있습니다.
 * 셔플에 사용하는 FRandomStream은 하나를 계속 사용하며, 스냅샷에는 현재 시드가 함께 기록됩니다.
 */
UCLASS()
class LETHE_API UCardContainerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCardContainerComponent();

	/**
	 * 장착 덱을 기준으로 카드 인스턴스를 생성하고 모든 카드를 덱에 넣습니다.
	 * 카드 데이터 로드나 Ability 부여와 무관하게 동기적으로 초기화됩니다.
	 */
	void Initialize(const TArray<FGameplayTag>& InCharacterOrder, const TMap<FGameplayTag, FSavedCharacterDeck>& EquippedDecks, const int32 InitialShuffleSeed);
	bool IsInitialized() const;

	/** 턴 페이즈 변화에 따라 핸드 버림, 덱 리필 및 셔플 등을 처리합니다. GameState가 호출합니다. */
	void HandleTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState, const bool bIsBattlePhase);

	//~ Begin Zone Transition
	/** 덱 맨 위 카드를 핸드 슬롯에 추가합니다. 핸드가 모두 채워지면 OnHandFull을 브로드캐스트합니다. */
	bool DrawTopCard(const FGameplayTag& CharacterTag, int32& OutInstanceId);
	/** 핸드에 있는 카드를 무덤으로 보냅니다. 슬롯은 유지되고 값만 INDEX_NONE이 됩니다. */
	bool MoveToGraveyard(const int32 InstanceId);
	/** 모든 캐릭터의 핸드 카드를 무덤으로 보내고 슬롯을 비웁니다. */
	void DiscardAllHands();
	/** 모든 캐릭터의 무덤을 덱으로 되돌립니다. 셔플은 별도로 수행해야 합니다. */
	void RefillAllDecks();
	void ShuffleAllDecks();
	//~ End of Zone Transition

	//~ Begin Query
	const FCardInstance* FindInstance(const int32 InstanceId) const;
	ECardZone GetZone(const int32 InstanceId) const;
	const FCharacterCardZones* GetZones(const FGameplayTag& CharacterTag) const;
	const TArray<FGameplayTag>& GetCharacterOrder() const;
	const TArray<FCardInstance>& GetInstances() const;

	/**
	 * 캐릭터 순서대로 이어붙인 핸드 슬롯을 반환합니다. 사용되어 빈 슬롯은 INDEX_NONE으로 포함됩니다.
	 * 드로우는 캐릭터 순서와 무관하게 진행되므로, 슬롯 인덱스 기반 접근은 항상 이 함수의 결과를 기준으로 합니다.
	 */
	void GetOrderedHandSlots(TArray<int32>& OutInstanceIds) const;
	int32 GetHandSlotCount() const;
	bool AreAllDecksEmpty() const;
	//~ End of Query

	//~ Begin Snapshot
	void Capture(FCardContainerSnapshot& OutSnapshot) const;
	void Restore(const FCardContainerSnapshot& InSnapshot);
	//~ End of Snapshot

private:
	FCharacterCardZones* FindZones(const FGameplayTag& CharacterTag);
	void BroadcastChanged() const;

public:
	FOnCardContainerChanged OnContainerChanged;
	FOnHandFull OnHandFull;

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Card")
	TArray<FGameplayTag> CharacterOrder;

	/** 배열 인덱스가 곧 InstanceId입니다. */
	UPROPERTY(VisibleInstanceOnly, Category = "Card")
	TArray<FCardInstance> Instances;

	UPROPERTY(VisibleInstanceOnly, Category = "Card")
	TMap<FGameplayTag, FCharacterCardZones> ZonesByCharacter;

	FRandomStream ShuffleStream;

	uint8 bInitialized : 1 = false;
};
