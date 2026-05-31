// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "CardLayoutManager.generated.h"

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
class LETHE_API UCardLayoutManager : public UObject
{
	GENERATED_BODY()

public:
	void Initialize(const FVector2D& CardSize, const FTransform& InLayoutTransform = FTransform::Identity);

	void AddCardToDeck(ACardActor* CardActor);
	void ShuffleDeck();
	ACardActor* GetTopCardFromDeck(ULetheAbilitySystemComponent* OwnerASC) const;
	bool TryDraw(ULetheAbilitySystemComponent* OwnerASC);

	void AddCardToGrave(ACardActor* CardActor);
	void AddAllHandsToGrave();

	void RefillDeck();

	/** 캐릭터 순서대로 순회할 필요가 있어 매개변수로 AbilitySystemComponents를 받아 ASCToCards에서 가져옵니다. */
	void MoveAllCards(const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& AbilitySystemComponents);

	bool AreAllDecksFull() const;
	bool AreAllDecksEmpty() const;

	const TArray<TObjectPtr<ACardActor>>& GetCurrentHands() const;
	int32 GetCurrentHandsNum() const;
	int32 FindCurrentHandIndex(ACardActor* CardActor) const;

private:
	void MoveCardToGraves(ACardActor* CardActor) const;

private:
	UPROPERTY()
	TMap<TObjectPtr<ULetheAbilitySystemComponent>, FCharacterCards> ASCToCards;

	UPROPERTY()
	TArray<TObjectPtr<ACardActor>> CurrentHands;

	FVector FirstCardLocation = FVector(80.f, -40.f, 0.f);
	FVector NextCardLocation = FVector(80.f, -40.f, 0.f);
	FVector GravesCardLocation = FVector(1760.f, -40.f, 0.f);
	FTransform LayoutTransform;
	float PaddingDeckAndHand = 25.f;
	float PaddingHandAndHand = 10.f;
};
