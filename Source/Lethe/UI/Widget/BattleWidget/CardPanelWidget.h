// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CardLayoutManager.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "CardPanelWidget.generated.h"

class UButton;
class UCanvasPanel;
class UCardPanelWidgetController;
class UCardUseSectionWidget;
class UCardWidget;
class ULetheAbilitySystemComponent;
struct FCardInitParams;
enum class ECardAction : uint8;

UCLASS()
class LETHE_API UCardPanelWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End of UUserWidget Interface

	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface

private:
	void OnMouseEvent(UCardWidget* CardWidget, const ECardAction CardAction);
	void OnMouseEventWhenDrawPhase(const UCardWidget* CardWidget, const ECardAction CardAction);
	void OnMouseEventWhenPlayerTurnPhase(UCardWidget* CardWidget, const ECardAction CardAction);
	void OnKeyboardEvent(const int32 Number);
	void OnKeyboardEventWhenDrawPhase(const int32 Number) const;
	void OnKeyboardEventWhenPlayerTurnPhase(const int32 Number);
	
	void CreateCard(const FCardInitParams& CardInitParams);
	void UpdateAllCardTranslation() const;
	void OnDeckHovered(const UCardWidget* CardWidget, const bool bHovered) const;
	void TryDraw(ULetheAbilitySystemComponent* OwnerASC) const;
	
	void OnHandHovered(UCardWidget* CardWidget, const bool bHovered) const;
	void SelectCard(UCardWidget* CardWidget);
	// 카드 사용을 위해 입력을 소비했다면 true를, 그렇지 않다면 false를 반환합니다.
	bool OnMouseButtonDownInCardUseSection() const;
	bool OnMouseButtonUpInCardUseSection();
	void ResetSelectedCard();
	void ResetSelectedCardWithoutEvent();

	void OnUseCardResolved(const int32 HandIndex, const bool bSuccess);

	UFUNCTION()
	void OnTurnEndButtonClicked();

	void OnPlayerPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState);
	void OnDrawPhaseStarted() const;
	
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

	UPROPERTY()
	TObjectPtr<UCardLayoutManager> CardLayoutManager;
	
	uint8 bControllerInitialized : 1 = false;

	EPhaseState CurrentPhaseState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UCardWidget> CurrentSelectedCard;
	
	UPROPERTY()
	TMap<int32, TObjectPtr<UCardWidget>> UseRequestedCards;

	uint8 bRightMouseButtonPressed : 1 = false;
};
