// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "CardPanelWidgetController.h"
#include "Animation/WidgetAnimation.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TimelineComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardSelfViewData.h"
#include "Lethe/Data/Card/CardViewData.h"
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

void UCardWidget::NativeDestruct()
{
	OnCardMouseEventDelegate.Unbind();
	
	Super::NativeDestruct();
}

void UCardWidget::SetSize(const FVector2D& InSize) const
{
	RootSizeBox->SetWidthOverride(InSize.X);
	RootSizeBox->SetHeightOverride(InSize.Y);
}

void UCardWidget::SetCardImageSize(const FVector2D& InCardImageSize, const float BaseRenderScale) const
{
	// 카드 확대 기능 수행을 위해 RenderScale을 1보다 낮은 수치로 할당했기 때문에, 디자이너탭에서 설정한 값들을 그대로 사용하면 문제가 생기므로 여기서 보정합니다.
	CardImage->SetDesiredSizeOverride(InCardImageSize);
	if (UOverlaySlot* CardImageSlot = Cast<UOverlaySlot>(CardImage->Slot))
	{
		FMargin DesiredPadding = CardImageSlot->GetPadding();
		DesiredPadding.Top /= BaseRenderScale;
		CardImageSlot->SetPadding(DesiredPadding);
	}
}

void UCardWidget::SetCardInfo(const FCardInitParams& InitParams)
{
	OwnerASC = InitParams.OwnerASC;
	CardTag = InitParams.CardDefinition->CardTag;
	CardImage->SetBrushFromTexture(InitParams.CardSelfViewData->CardTexture);
	CardName = InitParams.CardSelfViewData->CardNameText;
	CardFrontsideBorderImage->SetColorAndOpacity(*InitParams.CardViewData->FindCardTypeColor(InitParams.CardDefinition->CardTypeTag));
	CardBacksideBorderImage->SetColorAndOpacity(FLinearColor(InitParams.CharacterDefinitionData->CardBacksideColor));
	
	// 카드 크기 조정이 필요할 때, RenderScale을 1.f 이상 수치로 사용하면 텍스쳐, 텍스트가 깨져버립니다.
	// 그렇다고 CanvasPanelSlot을 사용하면 CanvasPanel이 CPU한테 염병을 떨기 때문에, Slot은 최대한 건드리지 않는 게 좋습니다.
	// 따라서 기본 사이즈를 1.f 미만 수치로 사용하고, 확대가 필요할 때 1.f로 설정합니다.
	BaseRenderScale = 1.f / InitParams.CardViewData->GetCardExpandScale();
	SetSize(InitParams.CardViewData->GetCardSize() / BaseRenderScale);
	SetCardImageSize(InitParams.CardViewData->GetCardImageSize() / BaseRenderScale, BaseRenderScale);
	SetRenderScale(FVector2D(BaseRenderScale));
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
	// 처리할 필요가 없는 경우 얼리 리턴합니다.
	if (CurrentCardContainer == InCardContainer)
	{
		return;
	}
	
	TurnOffHighlightOutline();
	
	CurrentCardContainer = InCardContainer;
	switch (InCardContainer)
	{
	case ECardContainer::Deck:
		SetRenderScale(FVector2D(BaseRenderScale));
		break;
	case ECardContainer::Hand:
		{
			PlayAnimation(ShowFrontAnimation, bShouldSkipAnimation ? ShowFrontAnimation->GetEndTime() : 0.f);
			bMouseHovered = false;

			if (!bShouldSkipAnimation)
			{
				// 드로우 직후엔 HandHovered 이벤트가 발생할 수 없도록 합니다.
				// 이 처리를 해주지 않으면 마우스가 이미 올라간 상태기 때문에, Hovered 이벤트는 발생하지 않고 UnHovered 이벤트만 발생해 플래그가 꼬입니다.
				bBlockHandHovered = true;
				FTimerHandle TimerHandle;
				TWeakObjectPtr<UCardWidget> WeakThis = this;
				GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakThis]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->bBlockHandHovered = false;
					}
				}, 0.5f, false, 0.5f);
			}
		}
		break;
	case ECardContainer::Selected:
		TurnOnHighlightOutline();
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

void UCardWidget::MouseHovered(const bool bInHovered)
{
	bMouseHovered = bInHovered;
	SetTargetTransform(TargetTransform);
}

void UCardWidget::TurnOnHighlightOutline() const
{
	OutlineImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UCardWidget::TurnOffHighlightOutline() const
{
	OutlineImage->SetVisibility(ESlateVisibility::Collapsed);
}

FGameplayTag UCardWidget::GetCardTag() const
{
	return CardTag;
}

ECardContainer UCardWidget::GetCurrentCardContainer() const
{
	return CurrentCardContainer;
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
	const FVector2D HoveredTranslation = bMouseHovered ? FVector2D(0.f, AddHoveredTranslation) : FVector2D::ZeroVector;
	const FVector2D LerpedTranslation = FMath::Lerp(StartTransform.Translation, TargetTransform.Translation + HoveredTranslation, InValue);
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
	if (bMouseHovered)
	{
		SetRenderTranslation(TargetTransform.Translation + FVector2D(0.f, AddHoveredTranslation));
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
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseButtonDown));
	}
	return FReply::Handled();
}

FReply UCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseButtonUp));
	}
	return FReply::Handled();
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
		GetCardActionWhenDeckState(InMouseEvent, CardAction);
		break;
	case ECardContainer::Hand:
		GetCardActionWhenHandState(InMouseEvent, CardAction);
		break;
	case ECardContainer::Selected:
		GetCardActionWhenSelectedState(InMouseEvent, CardAction);
		break;
	default:
		break;
	}

	return CardAction;
}

void UCardWidget::GetCardActionWhenDeckState(const ECardMouseEvent InMouseEvent, ECardAction& OutCardAction) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::MouseEnter:
		{
			// 덱 위에 마우스를 올려놓은 경우 들어오는 분기입니다.
			OutCardAction = ECardAction::DeckHovered;
		}
		break;
	case ECardMouseEvent::MouseLeave:
		{
			// 덱 위에서 마우스가 벗어날 때 들어오는 분기입니다.
			OutCardAction = ECardAction::DeckUnhovered;
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
			OutCardAction = ECardAction::Draw;
		}
		break;
	case ECardMouseEvent::MouseCaptureLost:
		{
			// 덱 상태로 마우스를 캡쳐하고 있던 중, 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
			OutCardAction = ECardAction::DeckUnhovered;
		}
		break;
	}
}

void UCardWidget::GetCardActionWhenHandState(const ECardMouseEvent InMouseEvent, ECardAction& OutCardAction) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::MouseEnter:
		{
			// 핸드 위에 마우스를 올려놓은 경우 들어오는 분기입니다.
			if (!bBlockHandHovered)
			{
				OutCardAction = ECardAction::HandHovered;
			}
		}
		break;
	case ECardMouseEvent::MouseLeave:
		{
			// 핸드 위에서 마우스가 벗어날 때 들어오는 분기입니다.
			if (!bBlockHandHovered)
			{
				OutCardAction = ECardAction::HandUnhovered;
			}
		}
		break;
	case ECardMouseEvent::MouseButtonDown:
		{
			// 핸드 위에서 마우스 버튼을 누를 때 들어오는 분기입니다.
		}
		break;
	case ECardMouseEvent::MouseButtonUp:
		{
			// 핸드 위에서 마우스 버튼을 뗄 때 들어오는 분기입니다.
			OutCardAction = ECardAction::Selected;
		}
		break;
	case ECardMouseEvent::MouseCaptureLost:
		{
			// 핸드 상태로 마우스를 캡쳐하고 있던 중, 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
			OutCardAction = ECardAction::HandUnhovered;
		}
		break;
	}
}

void UCardWidget::GetCardActionWhenSelectedState(const ECardMouseEvent InMouseEvent, ECardAction& OutCardAction) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::MouseEnter:
		{
			// 선택된 카드 위에 마우스를 올려놓은 경우 들어오는 분기입니다.
			if (!bBlockHandHovered)
			{
				OutCardAction = ECardAction::HandHovered;
			}
		}
		break;
	case ECardMouseEvent::MouseLeave:
		{
			// 선택된 카드 위에서 마우스가 벗어날 때 들어오는 분기입니다.
			if (!bBlockHandHovered)
			{
				OutCardAction = ECardAction::HandUnhovered;
			}
		}
	default:
		break;
	}
}
