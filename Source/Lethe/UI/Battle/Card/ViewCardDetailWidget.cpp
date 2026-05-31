// Copyright JETBLU, Inc. All Rights Reserved.

#include "ViewCardDetailWidget.h"

#include "CardPanelWidgetController.h"
#include "CardWidget.h"
#include "Components/Overlay.h"
#include "Lethe/Actor/Card/CardActor.h"
#include "Lethe/UI/Core/LetheRichTextBlock.h"

void UViewCardDetailWidget::StartViewDetail(const UCardWidget* InCardWidget)
{
	if (InCardWidget)
	{
		FViewDetailData ViewDetailData;
		InCardWidget->MakeViewDetailData(ViewDetailData);
		DetailCardWidget->SetViewDetail(ViewDetailData);

		FText OutDescriptionText;
		CardPanelWidgetController->GetCardDescriptionText(InCardWidget->GetOwnerASC(), InCardWidget->GetSavedCard(), OutDescriptionText);

		const FText FinalText = FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), ViewDetailData.CardNameText, OutDescriptionText);

		CardDescriptionTextBlock->SetText(FinalText);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UViewCardDetailWidget::StartViewDetail(const ACardActor* InCardActor)
{
	if (InCardActor)
	{
		FViewDetailData ViewDetailData;
		InCardActor->MakeViewDetailData(ViewDetailData);
		DetailCardWidget->SetViewDetail(ViewDetailData);

		FText OutDescriptionText;
		CardPanelWidgetController->GetCardDescriptionText(InCardActor->GetOwnerASC(), InCardActor->GetSavedCard(), OutDescriptionText);

		const FText FinalText = FText::Format(FText::FromString(TEXT("{0}\n\n{1}")), ViewDetailData.CardNameText, OutDescriptionText);
		CardDescriptionTextBlock->SetText(FinalText);
	}
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UViewCardDetailWidget::EndViewDetail()
{
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
