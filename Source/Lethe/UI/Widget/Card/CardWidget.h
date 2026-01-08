// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "CardWidget.generated.h"

class URichTextBlock;
struct FCardViewInfo;
class UImage;
class UAbilitySystemComponent;

// CardWidget이 현재 어디에 속해있는지 나타내는 Enum입니다.
// 이미 CardPanelWidget이 이를 알고는 있으나, CardWidget이 알고 있어야 스스로 어떤 CardAction이 발생했는지 판별한 후 CardPanelWidget에게 알려줄 수 있습니다.
UENUM()
enum class ECardContainer : uint8
{
	Deck,
	Hand,
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
	Use,

	None,
};

DECLARE_DELEGATE_TwoParams(FOnCardMouseEventSignature, class UCardWidget*, const ECardAction);

UCLASS()
class LETHE_API UCardWidget : public ULetheUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin UUserWidget Interface
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	//~ End of UUserWidget Interface
	
	void UpdateCardView(const FCardViewInfo* InCardInfo) const;

	void SetOwnerASC(UAbilitySystemComponent* InOwnerASC);
	UAbilitySystemComponent* GetOwnerASC() const;
	void SetCardContainer(const ECardContainer InCardPosition);
	bool ShouldHandHighlight() const;

private:
	ECardAction GetCardActionForEvent(const ECardMouseEvent InMouseEvent);

public:
	FOnCardMouseEventSignature OnCardMouseEventDelegate;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CardImage;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> CardNameTextBlock;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> CardDescriptionTextBlock;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> ShowFrontAnimation;

	UPROPERTY(meta = (BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> ShowBackAnimation;

private:
	ECardContainer CurrentCardContainer = ECardContainer::Deck;

	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	uint8 bReadyToDraw : 1 = false;
	uint8 bBlockHandHighlight : 1 = false;
	uint8 bShouldHandHighlight : 1 = false;
	uint8 bReadyToUse : 1 = false;
};
