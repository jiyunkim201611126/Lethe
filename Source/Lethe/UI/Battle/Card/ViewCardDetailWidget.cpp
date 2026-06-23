// Copyright JETBLU, Inc. All Rights Reserved.

#include "ViewCardDetailWidget.h"

#include "CardPanelWidgetController.h"
#include "CardWidget.h"
#include "Lethe/Actor/Card/CardActor.h"
#include "Lethe/UI/Core/LetheRichTextBlock.h"

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
	ActivateWidget();
}

void UViewCardDetailWidget::EndViewDetail()
{
	DeactivateWidget();
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

TOptional<FUIInputConfig> UViewCardDetailWidget::GetDesiredInputConfig() const
{
	FUIInputConfig Config(ECommonInputMode::Menu, EMouseCaptureMode::CaptureDuringMouseDown, false);
	Config.bIgnoreMoveInput = false;
	Config.bIgnoreLookInput = false;

	return Config;
}

void UViewCardDetailWidget::WidgetControllerSet_Implementation()
{
	CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController);
}
