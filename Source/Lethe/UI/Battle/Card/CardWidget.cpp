// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "CardPanelWidgetController.h"
#include "Animation/WidgetAnimation.h"
#include "Components/InvalidationBox.h"
#include "Components/SizeBox.h"
#include "Components/TimelineComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
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

void UCardWidget::SetCardInfo(const FCardInitParams& InitParams)
{
	OwnerASC = InitParams.OwnerASC;
	SavedCard = InitParams.SavedCard;
	CardNameText = InitParams.CardDefinition->CardNameText;
	CardImage->SetBrushFromTexture(InitParams.CardDefinition->CardTexture);

	const FLinearColor& CardTypeColor = InitParams.CardViewData->GetCardTypeColor(InitParams.CardDefinition->CardTypeTag);
	TypeFrameImage->SetColorAndOpacity(CardTypeColor);
	
	CardBacksideBorderImage->SetColorAndOpacity(FLinearColor(InitParams.CharacterDefinitionData->PersonalColor));
}

void UCardWidget::MakeViewDetailData(FViewDetailData& OutData) const
{
	OutData.CardNameText = CardNameText;
	OutData.CardImage = CardImage->GetBrush().GetResourceObject();
	OutData.CardTypeColor = TypeFrameImage->GetColorAndOpacity();
}

void UCardWidget::SetViewDetail(const FViewDetailData& InData)
{
	CardNameText = InData.CardNameText;
	CardImage->SetBrushResourceObject(InData.CardImage);
	TypeFrameImage->SetColorAndOpacity(InData.CardTypeColor);
	PlayAnimation(ShowFrontAnimation, ShowFrontAnimation->GetEndTime());
	FrontInvalidationBox->SetCanCache(false);
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
	switch (CurrentCardContainer)
	{
	case ECardContainer::Deck:
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
	if (bMouseHovered == bInHovered)
	{
		return;
	}
	
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
	return SavedCard.CardTag;
}

const FSavedCard& UCardWidget::GetSavedCard() const
{
	return SavedCard;
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

	FWidgetTransform NewTransform;
	NewTransform.Translation = LerpedTranslation;
	NewTransform.Angle = LerpedAngle;
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
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseLeave));
	bMouseButtonDown = false;
	
	Super::NativeOnMouseLeave(InMouseEvent);
}

FReply UCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseButtonDown));
	}
	bMouseButtonDown = true;
	
	return FReply::Handled();
}

FReply UCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bMouseButtonDown)
	{
		return FReply::Handled();
	}
	
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::LeftMouseButtonUp));
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::RightMouseButtonUp));
	}
	bMouseButtonDown = false;
	
	return FReply::Handled();
}

void UCardWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{	
	OnCardMouseEventDelegate.ExecuteIfBound(this, OnMouseEventForCardAction(ECardMouseEvent::MouseCaptureLost));
	bMouseButtonDown = false;
	
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
	case ECardMouseEvent::LeftMouseButtonUp:
		{
			// 덱 위에서 왼쪽 마우스 버튼을 뗄 때 들어오는 분기입니다.
			OutCardAction = ECardAction::Draw;
		}
		break;
	case ECardMouseEvent::RightMouseButtonUp:
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
	case ECardMouseEvent::LeftMouseButtonUp:
		{
			// 핸드 위에서 왼쪽 마우스 버튼을 뗄 때 들어오는 분기입니다.
			OutCardAction = ECardAction::Selected;
		}
		break;
	case ECardMouseEvent::RightMouseButtonUp:
		{
			// 핸드 위에서 오른쪽 마우스 버튼을 뗄 때 들어오는 분기입니다.
			OutCardAction = ECardAction::ViewDetail;
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
