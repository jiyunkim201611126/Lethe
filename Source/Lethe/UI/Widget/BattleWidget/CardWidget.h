// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/TimelineComponent.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "CardWidget.generated.h"

class ULetheAbilitySystemComponent;
class ULetheImage;
class USizeBox;
struct FCardInitParams;

// CardWidget이 현재 어디에 속해있는지 나타내는 Enum입니다.
// 이미 CardPanelWidget이 이를 알고는 있으나, CardWidget이 알고 있어야 스스로 어떤 CardAction이 발생했는지 판별한 후 CardPanelWidget에게 알려줄 수 있습니다.
UENUM()
enum class ECardContainer : uint8
{
	Deck,
	Hand,
	Selected,
	Grave,
};

// GetCardActionForEvent 함수를 통해 코드를 정리할 용도로 선언한 Enum입니다.
UENUM()
enum class ECardMouseEvent : uint8
{
	MouseEnter,
	MouseLeave,
	MouseButtonDown,
	MouseButtonUp,
	MouseCaptureLost
};

/**
 * Card가 어떤 Action을 취해야 하는지 나타내는 Enum입니다.
 * MouseEnter, MouseLeave등 마우스 자체의 Event를 사용하게 되면 CardPanelWidget의 Action 판단에 지저분한 로직을 작성해야 합니다.
 * 따라서 CardWidget이 스스로 Action을 판단한 후 CardPanelWidget에게 알려주도록 구성합니다.
 */
UENUM()
enum class ECardAction : uint8
{
	DeckHovered,
	DeckUnhovered,
	Draw,
	HandHovered,
	HandUnhovered,
	Selected,

	None,
};

DECLARE_DELEGATE_TwoParams(FOnCardMouseEventSignature, UCardWidget*, const ECardAction);

UCLASS()
class LETHE_API UCardWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
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

	void SetSize(const FVector2D& InSize) const;
	void SetCardImageSize(const FVector2D& InCardImageSize, const float BaseRenderScale) const;
	void SetCardInfo(const FCardInitParams& InitParams);

	ULetheAbilitySystemComponent* GetOwnerASC() const;

	// 현재 카드가 어떤 컨테이너에 속해있는지 전달받아 그에 따른 처리를 수행하는 함수입니다.
	void SetCardContainer(const ECardContainer InCardContainer, const bool bShouldSkipAnimation = false);
	
	// 마우스 이벤트에 의해 호출되는 함수로, 목표 지점을 결정한 뒤 이동을 시작합니다.
	void SetTargetTransform(const FWidgetTransform& InTransform);

	void MouseHovered(const bool bInHovered);

	FGameplayTag GetCardTag() const;

	ECardContainer GetCurrentCardContainer() const;

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
	TObjectPtr<ULetheImage> CardImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardFrontsideBorderImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> CardBacksideBorderImage;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheImage> OutlineImage;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> ShowFrontAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> ShowBackAnimation;

private:
	FText CardName;
	FGameplayTag CardTag;

	float BaseRenderScale = 0.5f;
	
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
};
