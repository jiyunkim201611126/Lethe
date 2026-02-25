// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "UObject/Object.h"
#include "CardLayoutManager.generated.h"

class UCanvasPanelSlot;
class UCardWidget;
class ULetheAbilitySystemComponent;

// TMap 컨테이너 내부에 TArray를 사용할 수 없는 문제를 우회하기 위한 구조체입니다.
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
	TArray<TObjectPtr<UCardWidget>> Deck;

	UPROPERTY()
	TArray<TObjectPtr<UCardWidget>> Hands;

	UPROPERTY()
	TArray<TObjectPtr<UCardWidget>> Graves;
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
	void Initialize(const FVector2D& CardSize);
	
	void SetupCardSlot(UCanvasPanelSlot* CardSlot) const;
	
	void AddCardToDeck(UCardWidget* CardWidget);
	UCardWidget* GetTopDeckCard(ULetheAbilitySystemComponent* OwnerASC) const;
	bool TryDraw(ULetheAbilitySystemComponent* OwnerASC);
	
	void OnCardSelected(const UCardWidget* CardWidget) const;
	void AddCardToGrave(UCardWidget* CardWidget);
	void AddAllHandsToGrave();

	// 캐릭터 순서대로 순회할 필요가 있어 매개변수로 AbilitySystemReferences를 받아 순회하면서 ASCToCards에서 가져옵니다.
	void MoveAllCards(const TArray<FAbilitySystemReference>& AbilitySystemReferences);
	
	bool AreAllDecksEmpty() const;
	
	const TArray<TObjectPtr<UCardWidget>>& GetCurrentHands() const;
	int32 GetCurrentHandsNum() const;
	int32 FindCurrentHandIndex(UCardWidget* CardWidget) const;

private:
	void MoveCardToGrave(UCardWidget* CardWidget) const;

private:
	UPROPERTY()
	TMap<TObjectPtr<ULetheAbilitySystemComponent>, FCharacterCards> ASCToCards;

	UPROPERTY()
	TArray<TObjectPtr<UCardWidget>> CurrentHands;

	int32 DeckZOrder = 100;
	int32 HandZOrder = 200;
	int32 SelectedZOrder = 300;

	FVector2D FirstCardTranslation = FVector2D(80.f, -40.f);
	FVector2D NextCardTranslation = FVector2D(80.f, -40.f);
	FVector2D GravesCardTranslation = FVector2D(1760.f, -40.f);
	float PaddingDeckAndHand = 25.f;
	float PaddingHandAndHand = 10.f;
};
