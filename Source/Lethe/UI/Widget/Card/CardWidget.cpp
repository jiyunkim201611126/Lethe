// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "Components/Image.h"
#include "Lethe/Data/CardViewData.h"

void UCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	
	OnCardMouseEventDelegate.Execute(this, GetCardActionForEvent(ECardMouseEvent::MouseEnter));
}

void UCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	OnCardMouseEventDelegate.Execute(this, GetCardActionForEvent(ECardMouseEvent::MouseLeave));
}

FReply UCardWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	GetCardActionForEvent(ECardMouseEvent::MouseButtonDown);
	
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
	OnCardMouseEventDelegate.Execute(this, GetCardActionForEvent(ECardMouseEvent::MouseButtonUp));
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCardWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{	
	OnCardMouseEventDelegate.Execute(this, GetCardActionForEvent(ECardMouseEvent::MouseCaptureLost));
	
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UCardWidget::UpdateCardView(const FCardViewInfo* InCardInfo) const
{
	if (InCardInfo)
	{
		CardImage->SetBrushFromTexture(InCardInfo->CardTexture);
	}
}

void UCardWidget::SetOwnerASC(UAbilitySystemComponent* InOwnerASC)
{
	OwnerASC = InOwnerASC;
}

UAbilitySystemComponent* UCardWidget::GetOwnerASC() const
{
	return OwnerASC.Get();
}

void UCardWidget::SetCardContainer(const ECardContainer InCardPosition)
{
	// 처리할 필요가 없는 경우 조기 return합니다.
	if (CurrentCardContainer == InCardPosition)
	{
		return;
	}
	
	CurrentCardContainer = InCardPosition;
	switch (InCardPosition)
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

bool UCardWidget::ShouldHandHighlight() const
{
	return bHandHighlight;
}

ECardAction UCardWidget::GetCardActionForEvent(const ECardMouseEvent InMouseEvent)
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
				CardAction = ECardAction::HandHovered;
				bHandHighlight = true;
			}
			break;
		case ECardMouseEvent::MouseLeave:
			{
				// 핸드 위에서 마우스가 벗어날 때 들어오는 분기입니다.
				if (bReadyToUse)
				{
					CardAction = ECardAction::None;
				}
				else
				{
					CardAction = ECardAction::HandUnhovered;
					bHandHighlight = false;
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
					bHandHighlight = false;
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
					bHandHighlight = false;
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
