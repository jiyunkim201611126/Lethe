// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardUseSectionWidget.h"

#include "Lethe/Player/PlayerController/LethePlayerController.h"

void UCardUseSectionWidget::NativeDestruct()
{
	Super::NativeDestruct();

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
	// 클릭 시 이 위젯이 Input을 캡쳐할 수 있도록 Handled를 반환합니다.
	return FReply::Handled();
}

FReply UCardUseSectionWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	OnMouseButtonUp.ExecuteIfBound();
	
	return FReply::Handled();
}
