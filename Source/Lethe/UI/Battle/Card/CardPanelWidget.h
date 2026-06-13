// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/PhaseData.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "CardPanelWidget.generated.h"

class ACardActor;
class ACardStage;
class UButton;
class UCanvasPanel;
class UCardPanelWidgetController;
class UCardUseSectionWidget;
class ULetheAbilitySystemComponent;
class ULetheImage;
class UViewCardDetailWidget;
struct FGameplayTag;
struct FCardInitParams;
struct FSavedCard;

UCLASS(Abstract)
class LETHE_API UCardPanelWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	//~ End of UUserWidget Interface

private:
	/**  */
	void TryInitializeCardStage() const;
	
	/**
	 * CapturedCardStage의 UV를 기준으로 현재 마우스 위치가 어디인지 계산합니다.
	 * 반환값은 마우스가 CapturedCardStage 위 Hovered 상태 여부입니다.
	 */
	bool TryGetCapturedCardStageUV(const FPointerEvent& InMouseEvent, FVector2D& OutUV) const;

	void CreateCard(const FCardInitParams& CardInitParams) const;
	
	void OnKeyboardEvent(int32 Number) const;
	bool OnMouseButtonDownInCardUseSection() const;
	bool OnMouseButtonUpInCardUseSection() const;
	void OnCancelSelectedCard() const;
	void OnResolveUseCard(int32 HandIndex, bool bSuccess) const;
	void StartViewCardDetail(const ACardActor* CardActor) const;
	bool SetCardSelected(bool bCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag) const;
	void GoPlayerTurnPhase() const;
	void StartResolvePlayerMoves() const;
	void RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, int32 HandIndex) const;
	bool RequestTurnEnd() const;

	UFUNCTION()
	void OnTurnEndButtonClicked();

	void OnPhaseStateChanged(EPhaseState OldState, EPhaseState NewState) const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCanvasPanel> RootCanvasPanel;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCardUseSectionWidget> CardUseSection;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TurnEndButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UViewCardDetailWidget> ViewCardDetail;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CapturedCardStage;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ACardStage> CardStageClass;

private:
	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;

	UPROPERTY()
	TObjectPtr<ACardStage> CardStage;
};
