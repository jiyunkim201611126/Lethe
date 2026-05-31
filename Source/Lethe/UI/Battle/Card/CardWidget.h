// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/TimelineComponent.h"
#include "Lethe/Actor/Card/CardActor.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "CardWidget.generated.h"

enum class ECardMouseEvent : uint8;
enum class ECardContainer : uint8;
class UInvalidationBox;
class ULetheAbilitySystemComponent;
class ULetheImage;
class USizeBox;
struct FCardInitParams;

DECLARE_DELEGATE_TwoParams(FOnCardMouseEventSignature, UCardWidget*, const ECardAction);

UCLASS()
class LETHE_API UCardWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	void SetSize(const FVector2D& InSize) const;
	void SetCardInfo(const FCardInitParams& InitParams);

	void MakeViewDetailData(FViewDetailData& OutData) const;
	void SetViewDetail(const FViewDetailData& InData);

	ULetheAbilitySystemComponent* GetOwnerASC() const;

	/** 현재 카드가 어떤 컨테이너에 속해있는지 전달받아 그에 따른 처리를 수행하는 함수입니다. */
	void SetCardContainer(const ECardContainer InCardContainer, const bool bShouldSkipAnimation = false);
	
	/** 마우스 이벤트에 의해 호출되는 함수로, 목표 지점을 결정한 뒤 이동을 시작합니다. */
	void SetTargetTransform(const FWidgetTransform& InTransform);

	void MouseHovered(const bool bInHovered);

	FGameplayTag GetCardTag() const;
	const FSavedCard& GetSavedCard() const;

	ECardContainer GetCurrentCardContainer() const;

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, const float InDeltaTime) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	//~ End of UUserWidget Interface

private:
	UFUNCTION()
	void OnUpdatedTimeline(const float InValue);
	
	UFUNCTION()
	void OnFinishedTimeline();

	void TurnOnHighlightOutline() const;
	void TurnOffHighlightOutline() const;

	ECardAction OnMouseEventForCardAction(const ECardMouseEvent InMouseEvent) const;
	void GetCardActionWhenDeckState(const ECardMouseEvent InMouseEvent, ECardAction& OutCardAction) const;
	void GetCardActionWhenHandState(const ECardMouseEvent InMouseEvent, ECardAction& OutCardAction) const;
	void GetCardActionWhenSelectedState(const ECardMouseEvent InMouseEvent, ECardAction& OutCardAction) const;

public:
	FOnCardMouseEventSignature OnCardMouseEventDelegate;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<USizeBox> RootSizeBox;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UInvalidationBox> FrontInvalidationBox;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardFrontsideBorderImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> TypeFrameImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> SortFrameImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardBacksideBorderImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardBacksideImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> OutlineImage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> ShowFrontAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> ShowBackAnimation;

private:
	FText CardNameText;
	FSavedCard SavedCard;
	
	TWeakObjectPtr<ULetheAbilitySystemComponent> OwnerASC;

	FTimeline MovementTimeline;
	
	UPROPERTY(EditAnywhere, Category = "Animation")
	TObjectPtr<UCurveFloat> MovementCurve;

	FWidgetTransform StartTransform;
	FWidgetTransform TargetTransform;

	float AddHoveredTranslation = -10.f;
	
	ECardContainer CurrentCardContainer = ECardContainer::Deck;

	uint8 bShouldMove : 1 = false;
	uint8 bBlockHandHovered : 1 = false;
	uint8 bMouseHovered : 1 = false;

	uint8 bMouseButtonDown : 1 = false;
};
