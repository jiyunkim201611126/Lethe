// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CardContainerManager.generated.h"

class ADeckBoxes;
class ACardActor;
class ULetheAbilitySystemComponent;

/**
 * 드로우하며 정렬된 덱과 카드들은 카드 사용 시점이 아닌 턴 종료 시에 위치가 재정렬됩니다.
 * 따라서 사용한 후 카드가 무덤으로 이동했어도, Hand 배열의 해당 카드 위치였던 곳은 nullptr로 두어 위치를 잡는 로직의 일부로 사용해야 합니다.
 * 하지만 Hand 배열 안에 nullptr가 들어있는 게 오류라고 오해할 여지가 있으므로, HandSlot 구조체를 통해 읽는이로 하여금 nullptr도 로직의 일부임을 분명하게 보여줍니다.
 */
USTRUCT()
struct FHandSlot
{
	GENERATED_BODY()

	bool IsEmpty() const
	{
		return !Card;
	}

	ACardActor* GetCard() const
	{
		return Card;
	}

	void SetCard(ACardActor* InCard)
	{
		Card = InCard;
	}

	void Clear()
	{
		Card = nullptr;
	}

private:
	/** 카드가 사용되어도 슬롯 자체는 유지되며, 이 값만 nullptr이 될 수 있습니다. */
	UPROPERTY()
	TObjectPtr<ACardActor> Card = nullptr;
};

/** TMap 컨테이너 내부에 TArray를 사용할 수 없는 문제를 우회하기 위한 구조체입니다. */
USTRUCT()
struct FCharacterCards
{
	GENERATED_BODY()

	FCharacterCards()
	{
		Deck.Reserve(10);
		HandSlots.Reserve(8);
		DeckPreviewCards.Reserve(10);
		Graveyard.Reserve(10);
	}

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> Deck;

	/** 드로우한 카드가 배치된 슬롯 배열입니다. 카드 사용 시 슬롯은 유지되고 Card만 nullptr이 됩니다. */
	UPROPERTY()
	TArray<FHandSlot> HandSlots;

	/** 비전투 중 덱 열람 시 표시할 카드 목록입니다. */
	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> DeckPreviewCards;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> Graveyard;

	void SortDeckPreviewCards();
};

/**
 * Card 상태(Deck/Hand/Grave)와 레이아웃을 함께 담당하는 클래스입니다.
 * Add는 로직상의 추가를, Move는 카드를 View상에서 움직임을 의미합니다.
 */
UCLASS()
class LETHE_API UCardContainerManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents, ADeckBoxes* InDeckBoxes);

	void AddCardToDeck(ACardActor* CardActor);
	void ShuffleDeck();
	ACardActor* GetTopCardFromDeck(ULetheAbilitySystemComponent* OwnerASC) const;
	bool AddCardToHandSlot(ULetheAbilitySystemComponent* OwnerASC);

	void AddCardToGraveyard(ACardActor* CardActor);
	void AddAllHandSlotsToGraveyard();

	void RefillDeck();

	void MoveAllCards();

	void PreviewDeck(ULetheAbilitySystemComponent* DeckOwnerASC);
	void StopPreviewDeck(const bool bShouldMoveCards = true);

	bool AreAllDecksFull() const;
	bool AreAllDecksEmpty() const;

	const TArray<FHandSlot>& GetCurrentHandSlots() const;
	int32 GetCurrentHandSlotCount() const;
	int32 FindCurrentHandSlotIndex(const ACardActor* CardActor) const;

private:
	void GetCurrentHandSlotCounts(TArray<int32>& OutHandSlotCounts);

private:
	/** CardStage와 마찬가지로, 캐릭터 순서대로 접근하기 위해 AbilitySystemComponent 배열을 캐싱해둡니다. */
	TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;
	TMap<TWeakObjectPtr<ULetheAbilitySystemComponent>, FCharacterCards> ASCToCards;

	TWeakObjectPtr<ADeckBoxes> DeckBoxes;

	UPROPERTY()
	TArray<FHandSlot> CurrentHandSlots;

	FVector DeckFirstCardLocation = FVector(-4.9f, 0.3f, 3.82f);
	float DeckCardXOffset = 0.7f;

	FVector HandSlotFirstCardLocation = FVector(1.f, -2.80185f, 3.8543f);
	float HandSlotCardXOffset = 8.f;

	FVector GraveyardFirstCardLocation = FVector(0.f, -2.8f, 5.58038f);
	FVector GraveyardCardOffset = FVector(0.f, -0.3f, 0.51962f);
};
