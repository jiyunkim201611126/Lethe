// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardUseSectionWidget.h"

#include "Lethe/Player/PlayerController/LethePlayerController.h"

void UCardUseSectionWidget::NativeDestruct()
{
	Super::NativeDestruct();

	OnMouseButtonDown.Unbind();
	OnMouseButtonUp.Unbind();
}

void UCardUseSectionWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
	{
		PlayerController->SetMouseOnCardUseSection(true);
	}
}

void UCardUseSectionWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
	{
		PlayerController->SetMouseOnCardUseSection(false);
	}
}

FReply UCardUseSectionWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 마우스 왼쪽 클릭 시 이 위젯이 Input을 캡쳐할 수 있도록 Handled를 반환합니다.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnMouseButtonDown.IsBound())
	{
		if (OnMouseButtonDown.Execute())
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}

FReply UCardUseSectionWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && OnMouseButtonUp.IsBound())
	{
		// 카드를 선택한 상태에서 입력이 수행되었다면 true를 반환받아 입력을 소비, 그렇지 않다면 Unhandled를 반환해 PlayerController까지 입력을 내려줍니다.
		if (OnMouseButtonUp.Execute())
		{
			return FReply::Handled();
		}
	}
	return FReply::Unhandled();
}
