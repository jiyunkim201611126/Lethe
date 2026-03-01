// Copyright JETBLU, Inc. All Rights Reserved.

#include "ViewCardDetailWidget.h"

#include "CardPanelWidgetController.h"
#include "CardWidget.h"
#include "Components/Overlay.h"
#include "Lethe/UI/Core/LetheImage.h"
#include "Lethe/UI/Core/LetheRichTextBlock.h"

void UViewCardDetailWidget::StartViewDetail(const UCardWidget* InCardWidget)
{
	if (InCardWidget)
	{
		FViewDetailData ViewDetailData;
		InCardWidget->TryMakeViewDetailData(ViewDetailData);
		
		CardOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CardImage->SetBrushResourceObject(ViewDetailData.CardImage);
		CardFrontsideBorderImage->SetColorAndOpacity(ViewDetailData.CardFrontsideBorderColor);

		FText OutDescriptionText;
		CardPanelWidgetController->GetCardDescriptionText(InCardWidget->GetOwnerASC(), InCardWidget->GetCardTag(), OutDescriptionText);

		const FText FinalText = FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), ViewDetailData.CardNameText, OutDescriptionText);

		CardDescriptionTextBlock->SetText(FinalText);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UViewCardDetailWidget::EndViewDetail()
{
	CardOverlay->SetVisibility(ESlateVisibility::Collapsed);
	SetVisibility(ESlateVisibility::Collapsed);
}

FReply UViewCardDetailWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	return FReply::Handled();
}

FReply UViewCardDetailWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		EndViewDetail();
	}
	return FReply::Handled();
}

void UViewCardDetailWidget::WidgetControllerSet_Implementation()
{
	CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController);
}
