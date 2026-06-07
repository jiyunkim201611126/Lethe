// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CardContainerManager.generated.h"

class ADeckBoxes;
class ACardActor;
class ULetheAbilitySystemComponent;

/** TMap 컨테이너 내부에 TArray를 사용할 수 없는 문제를 우회하기 위한 구조체입니다. */
USTRUCT(BlueprintType)
struct FCharacterCards
{
	GENERATED_BODY()

	FCharacterCards()
	{
		Deck.Reserve(10);
		Hands.Reserve(8);
		Graves.Reserve(10);
	}

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> Deck;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> Hands;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> Graves;
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
	bool TryDraw(ULetheAbilitySystemComponent* OwnerASC);

	void AddCardToGrave(ACardActor* CardActor);
	void AddAllHandsToGrave();

	void RefillDeck();

	void MoveAllCards();

	bool AreAllDecksFull() const;
	bool AreAllDecksEmpty() const;

	const TArray<TObjectPtr<ACardActor>>& GetCurrentHands() const;
	int32 GetCurrentHandCount() const;
	int32 FindCurrentHandIndex(ACardActor* CardActor) const;

private:
	void GetCurrentHandCounts(TArray<int32>& HandCounts);

private:
	/** CardStage와 마찬가지로, 캐릭터 순서대로 접근하기 위해 AbilitySystemComponent 배열을 캐싱해둡니다. */
	TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;
	TMap<TWeakObjectPtr<ULetheAbilitySystemComponent>, FCharacterCards> ASCToCards;

	TWeakObjectPtr<ADeckBoxes> DeckBoxes;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> CurrentHands;

	FVector DeckFirstCardLocation = FVector(-4.9f, 0.3f, -0.3f);
	float DeckCardXOffset = 0.7f;

	FVector HandFirstCardLocation = FVector(1.f, -2.80185f, -0.2657f);
	float HandCardXOffset = 8.f;

	FVector GravesFirstCardLocation = FVector(0.f, -6.7f, 3.48f);
	FVector GravesCardOffset = FVector(0.f, -0.3f, 0.51962f);
};
