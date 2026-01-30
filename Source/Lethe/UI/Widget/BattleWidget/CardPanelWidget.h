// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "Lethe/UI/WidgetController/LetheWidgetController.h"
#include "CardPanelWidget.generated.h"

class UCardUseSectionWidget;
class UButton;
struct FCardInitParams;
class UCardPanelWidgetController;
class UCardWidget;
class UCanvasPanel;
class ULetheAbilitySystemComponent;
enum class ECardAction : uint8;

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

UCLASS()
class LETHE_API UCardPanelWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	//~ End of UUserWidget Interface

	virtual void WidgetControllerSet_Implementation() override;

private:
	void OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction);	
	void OnKeyboardEvent(const int32 InNumber);
	void OnKeyboardEventWhenDrawPhase(const int32 InNumber);
	void OnKeyboardEventWhenBattlePhase(const int32 InNumber);
	
	void CreateCard(const FCardInitParams& CardInitParams);
	void UpdateAllCardTranslation();
	void OnDeckHovered(const UCardWidget* InCardWidget, const bool bInHovered);
	void Draw(const UCardWidget* InCardWidget);
	
	void OnHandHovered(UCardWidget* InCardWidget, const bool bInHovered) const;
	void ReadyToUseCard(UCardWidget* InCardWidget, const bool bByMouseEvent);
	void ResetReadyToUseCard();
	void TryUseCard();
	void UseCardSuccess();
	void UseCardFail();

	UFUNCTION()
	void OnTurnEndButtonClicked();

	void OnPlayerPhaseStateChanged(const EPlayerPhaseState InState);
	void OnDrawPhaseStarted();
	
protected:
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TSubclassOf<UUserWidget> CardWidgetClass;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvasPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCardUseSectionWidget> CardUseSection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TurnEndButton;

private:
	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;
	
	// Key를 OwnerASC로, Value로 CardWidget(Deck)을 매핑한 변수입니다.
	// 이미 CardWidget도 OwnerASC를 멤버 변수로 갖고 있으나, CardPanelWidget도 정렬 및 접근 효율을 위해 매핑해두는 편이 좋습니다.
	UPROPERTY()
	TMap<TObjectPtr<ULetheAbilitySystemComponent>, FCharacterCards> AbilitySystemComponentToCards;
	
	uint8 bControllerInitialized : 1 = false;

	EPlayerPhaseState CurrentPlayerPhaseState = EPlayerPhaseState::None;

	int32 DeckZOrder = 100;
	int32 HandZOrder = 200;
	int32 DraggingZOrder = 300;

	FVector2D FirstCardTranslation = FVector2D(80.f, -40.f);
	FVector2D NextCardTranslation = FVector2D(80.f, -40.f);
	FVector2D GravesCardTranslation = FVector2D(1760.f, -40.f);
	float PaddingDeckAndHand = 25.f;
	float PaddingHandAndHand = 10.f;
	float CardHighlightScale;

	TWeakObjectPtr<UCardWidget> CurrentReadyToUseCard;

	// 키보드 입력으로 조작 시 핸드를 빠르게 탐색하기 위해 선언한 변수입니다.
	UPROPERTY()
	TArray<TObjectPtr<UCardWidget>> CurrentHands;
};
