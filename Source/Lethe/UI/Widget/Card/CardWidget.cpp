// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "Components/Image.h"
#include "Lethe/Data/CardViewData.h"
#include "Components/RichTextBlock.h"
#include "Components/TimelineComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"

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

void UCardWidget::SetCardContainer(const ECardContainer InCardContainer)
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
		break;
	case ECardContainer::Hand:
		PlayAnimation(ShowFrontAnimation);
		break;
	case ECardContainer::Grave:
		PlayAnimation(ShowBackAnimation);
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

void UCardWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	if (!bShouldMove)
	{
		return;
	}
	
	MovementTimeline.TickTimeline(InDeltaTime);
}

void UCardWidget::OnUpdatedTimeline(float InValue)
{
	const FVector2D LerpedTranslation = FMath::Lerp(StartTransform.Translation, TargetTransform.Translation, InValue);
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
	SetRenderTransform(TargetTransform);
}

void UCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	OnCardMouseEventDelegate.Execute(this, OnCardActionForEvent(ECardMouseEvent::MouseEnter));
}

void UCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	OnCardMouseEventDelegate.Execute(this, OnCardActionForEvent(ECardMouseEvent::MouseLeave));
}

FReply UCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnCardActionForEvent(ECardMouseEvent::MouseButtonDown);
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCardWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bReadyToUse)
	{
		// TODO: 카드가 마우스를 따라다녀야 함
	}
	
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UCardWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnCardMouseEventDelegate.Execute(this, OnCardActionForEvent(ECardMouseEvent::MouseButtonUp));
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCardWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{	
	OnCardMouseEventDelegate.Execute(this, OnCardActionForEvent(ECardMouseEvent::MouseCaptureLost));
	
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UCardWidget::UpdateCardView(const FCardViewInfo* InCardInfo) const
{
	if (InCardInfo)
	{
		CardImage->SetBrushFromTexture(InCardInfo->CardTexture);
		CardNameTextBlock->SetText(InCardInfo->CardNameText);
		CardDescriptionTextBlock->SetText(InCardInfo->CardDescriptionText);
	}
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

bool UCardWidget::GetHandHighlightState() const
{
	return bShouldHandHighlight;
}

ECardAction UCardWidget::OnCardActionForEvent(const ECardMouseEvent InMouseEvent)
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
				bReadyToDraw = false;
			}
			break;
		case ECardMouseEvent::MouseButtonDown:
			{
				// 덱 위에서 마우스 버튼을 누를 때 들어오는 분기입니다.
				bReadyToDraw = true;
			}
			break;
		case ECardMouseEvent::MouseButtonUp:
			{
				// 덱에서 마우스 버튼을 뗄 때 들어오는 분기입니다.
				if (bReadyToDraw)
				{
					CardAction = ECardAction::Draw;
					bReadyToDraw = false;

					// 드로우 직후엔 HandHovered 이벤트가 발생할 수 없도록 합니다.
					// 마우스가 이미 올라간 상태기 때문에, Hovered 이벤트는 발생하지 않고 UnHovered 이벤트만 발생합니다.
					bBlockHandHighlight = true;
					FTimerHandle TimerHandle;
					TWeakObjectPtr<UCardWidget> WeakThis = this;
					GetWorld()->GetTimerManager().SetTimer(TimerHandle, [WeakThis]()
					{
						if (WeakThis.IsValid())
						{
							WeakThis->bBlockHandHighlight = false;
						}
					}, 0.2f, false, 0.2f);
				}
			}
			break;
		case ECardMouseEvent::MouseCaptureLost:
			{
				// 덱 상태로 마우스를 캡쳐하고 있던 중, 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
				if (bReadyToDraw)
				{
					CardAction = ECardAction::DeckUnhovered;
					bReadyToDraw = false;
				}
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
					bShouldHandHighlight = true;
				}
			}
			break;
		case ECardMouseEvent::MouseLeave:
			{
				// 핸드 위에서 마우스가 벗어날 때 들어오는 분기입니다.
				if (!bBlockHandHighlight)
				{
					if (bReadyToUse)
					{
						CardAction = ECardAction::None;
					}
					else
					{
						CardAction = ECardAction::HandUnhovered;
						bShouldHandHighlight = false;
					}
				}
			}
			break;
		case ECardMouseEvent::MouseButtonDown:
			{
				// 핸드 위에서 마우스 버튼을 누를 때 들어오는 분기입니다.
				bReadyToUse = true;
			}
			break;
		case ECardMouseEvent::MouseButtonUp:
			{
				// 핸드에서 마우스 버튼을 뗄 때 들어오는 분기입니다.
				if (bReadyToUse)
				{
					CardAction = ECardAction::Use;
					bReadyToUse = false;
					bShouldHandHighlight = false;
				}
			}
			break;
		case ECardMouseEvent::MouseCaptureLost:
			{
				// 핸드 상태로 마우스를 캡쳐하고 있던 중, 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
				if (bReadyToUse)
				{
					CardAction = ECardAction::HandUnhovered;
					bReadyToUse = false;
					bShouldHandHighlight = false;
				}
			}
			break;
		}
		break;
	default:
		break;
	}

	return CardAction;
}
