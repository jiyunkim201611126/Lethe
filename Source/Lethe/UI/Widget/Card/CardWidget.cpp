// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "Animation/WidgetAnimation.h"
#include "Components/SizeBox.h"
#include "Lethe/Data/CardViewData.h"
#include "Components/TimelineComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/UI/Core/LetheImage.h"

void UCardWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 타임라인에 맞춰 카드가 움직일 수 있도록 함수들을 바인드합니다.
	FOnTimelineFloat OnUpdateFunction;
	OnUpdateFunction.BindDynamic(this, &ThisClass::OnUpdatedTimeline);
	MovementTimeline.AddInterpFloat(MovementCurve, OnUpdateFunction);

	FOnTimelineEvent OnFinishedFunction;
	OnFinishedFunction.BindDynamic(this, &ThisClass::OnFinishedTimeline);
	MovementTimeline.SetTimelineFinishedFunc(OnFinishedFunction);
}

void UCardWidget::SetSize(const FVector2D& InSize) const
{
	RootSizeBox->SetWidthOverride(InSize.X);
	RootSizeBox->SetHeightOverride(InSize.Y);
}

void UCardWidget::SetCardInfo(const FGameplayTag& InCardTag, const FCardSelfViewInfo* InCardViewInfo, const FCardOwnerViewInfo* InCardOwnerViewInfo)
{
	CardTag = InCardTag;
	if (InCardViewInfo)
	{
		CardImage->SetBrushFromTexture(InCardViewInfo->CardTexture);
		CardName = InCardViewInfo->CardNameText;
		CardDescription = InCardViewInfo->CardDescriptionText;
	}
	CardFrontsideBorderImage->SetColorAndOpacity(FLinearColor(InCardOwnerViewInfo->CardFrontsideColor));
	CardBacksideBorderImage->SetColorAndOpacity(FLinearColor(InCardOwnerViewInfo->CardBacksideColor));
}

void UCardWidget::SetOwnerASC(ULetheAbilitySystemComponent* InOwnerASC)
{
	OwnerASC = InOwnerASC;
}

ULetheAbilitySystemComponent* UCardWidget::GetOwnerASC() const
{
	if (OwnerASC.IsValid())
	{
		return OwnerASC.Get();
	}
	
	return nullptr;
}

void UCardWidget::SetCardContainer(const ECardContainer InCardContainer, const bool bShouldSkipAnimation)
{
	// 처리할 필요가 없는 경우 조기 return합니다.
	if (CurrentCardContainer == InCardContainer)
	{
		return;
	}
	
	CurrentCardContainer = InCardContainer;
	switch (InCardContainer)
	{
	case ECardContainer::Deck:
		SetRenderScale(FVector2D(0.5f));
		break;
	case ECardContainer::Hand:
		{
			PlayAnimation(ShowFrontAnimation, bShouldSkipAnimation ? ShowFrontAnimation->GetEndTime() : 0.f);
			bIsDragging = false;
			bCardHighlight = false;
			bBlockHandHighlight = true;

			// Hand가 된 직후엔 HandHovered 이벤트가 발생할 수 없도록 합니다.
			// 이 처리를 해주지 않으면 마우스가 이미 올라간 상태기 때문에, Hovered 이벤트는 발생하지 않고 UnHovered 이벤트만 발생해 플래그가 꼬입니다.
			FTimerHandle TimerHandle;
			TWeakObjectPtr<UCardWidget> WeakThis = this;
			GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakThis]()
			{
				if (WeakThis.IsValid())
				{
					WeakThis->bBlockHandHighlight = false;
				}
			}, 0.5f, false, 0.5f);
		}
		break;
	case ECardContainer::Dragging:
		// 카드 사용 준비 상태인 경우 들어오는 분기입니다.
		bIsDragging = true;
		bShouldMove = false;
		bCardHighlight = false;
		break;
	case ECardContainer::Grave:
		// 카드 사용 후 들어오는 분기입니다.
		PlayAnimation(ShowBackAnimation, bShouldSkipAnimation ? ShowBackAnimation->GetEndTime() : 0.f);
		break;
	}
}

void UCardWidget::SetTargetTransform(const FWidgetTransform& InTransform)
{
	StartTransform = GetRenderTransform();
	TargetTransform = InTransform;

	bShouldMove = true;
	MovementTimeline.PlayFromStart();
}

void UCardWidget::HighlightCard(const bool bInHighlight)
{
	if (bCardHighlight == bInHighlight)
	{
		return;
	}

	bCardHighlight = bInHighlight;
	SetTargetTransform(TargetTransform);
}

bool UCardWidget::IsDragging() const
{
	return bIsDragging;
}

FGameplayTag UCardWidget::GetCardTag() const
{
	return CardTag;
}

void UCardWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (bShouldMove)
	{
		MovementTimeline.TickTimeline(InDeltaTime);
	}
}

void UCardWidget::OnUpdatedTimeline(const float InValue)
{
	const FVector2D HighlightTranslation = bCardHighlight ? FVector2D(0.f, AddHighlightTranslation) : FVector2D::ZeroVector;
	const FVector2D LerpedTranslation = FMath::Lerp(StartTransform.Translation, TargetTransform.Translation + HighlightTranslation, InValue);
	const float LerpedAngle = FMath::Lerp(StartTransform.Angle, TargetTransform.Angle, InValue);
	const FVector2D LerpedScale = FMath::Lerp(StartTransform.Scale, TargetTransform.Scale, InValue);

	FWidgetTransform NewTransform;
	NewTransform.Translation = LerpedTranslation;
	NewTransform.Angle = LerpedAngle;
	NewTransform.Scale = LerpedScale;
	NewTransform.Shear = FVector2D::Zero();

	SetRenderTransform(NewTransform);
}

void UCardWidget::OnFinishedTimeline()
{
	bShouldMove = false;
	StartTransform = TargetTransform;
	SetRenderTransform(TargetTransform);
	if (bCardHighlight)
	{
		SetRenderTranslation(TargetTransform.Translation + FVector2D(0.f, AddHighlightTranslation));
	}
}

void UCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseEnter));
}

void UCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseLeave));
}

FReply UCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseButtonDown));
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseButtonUp));
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCardWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{	
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseCaptureLost));
	
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

ECardAction UCardWidget::OnMouseEventForCardAction(const ECardMouseEvent InMouseEvent) const
{
	ECardAction CardAction = ECardAction::None;

	switch (CurrentCardContainer)
	{
	case ECardContainer::Deck:
		switch (InMouseEvent)
		{
		case ECardMouseEvent::MouseEnter:
			{
				// 덱 위에 마우스를 올려놓은 경우 들어오는 분기입니다.
				CardAction = ECardAction::DeckHovered;
			}
			break;
		case ECardMouseEvent::MouseLeave:
			{
				// 덱 위에서 마우스가 벗어날 때 들어오는 분기입니다.
				CardAction = ECardAction::DeckUnhovered;
			}
			break;
		case ECardMouseEvent::MouseButtonDown:
			{
				// 덱 위에서 마우스 버튼을 누를 때 들어오는 분기입니다.
			}
			break;
		case ECardMouseEvent::MouseButtonUp:
			{
				// 덱 위에서 마우스 버튼을 뗄 때 들어오는 분기입니다.
				CardAction = ECardAction::Draw;
			}
			break;
		case ECardMouseEvent::MouseCaptureLost:
			{
				// 덱 상태로 마우스를 캡쳐하고 있던 중, 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
				CardAction = ECardAction::DeckUnhovered;
			}
			break;
		}
		break;
		
	case ECardContainer::Hand:
		switch (InMouseEvent)
		{
		case ECardMouseEvent::MouseEnter:
			{
				// 핸드 위에 마우스를 올려놓은 경우 들어오는 분기입니다.
				if (!bBlockHandHighlight)
				{
					CardAction = ECardAction::HandHovered;
				}
			}
			break;
		case ECardMouseEvent::MouseLeave:
			{
				// 핸드 위에서 마우스가 벗어날 때 들어오는 분기입니다.
				if (!bBlockHandHighlight)
				{
					CardAction = bIsDragging ? ECardAction::None : ECardAction::HandUnhovered;
				}
			}
			break;
		case ECardMouseEvent::MouseButtonDown:
			{
				// 핸드 위에서 마우스 버튼을 누를 때 들어오는 분기입니다.
				CardAction = ECardAction::Drag;
			}
			break;
		case ECardMouseEvent::MouseButtonUp:
			{
				// 핸드 위에서 마우스 버튼을 뗄 때 들어오는 분기입니다.
			}
			break;
		case ECardMouseEvent::MouseCaptureLost:
			{
				// 핸드 상태로 마우스를 캡쳐하고 있던 중, 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
				CardAction = ECardAction::HandUnhovered;
			}
			break;
		}
		break;
	default:
		break;
	}

	return CardAction;
}
