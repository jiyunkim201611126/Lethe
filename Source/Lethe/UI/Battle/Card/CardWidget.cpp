// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "CardPanelWidgetController.h"
#include "Components/InvalidationBox.h"
#include "Components/SizeBox.h"
#include "Lethe/Actor/Card/CardActor.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/UI/Core/LetheImage.h"

void UCardWidget::SetSize(const FVector2D& InSize) const
{
	RootSizeBox->SetWidthOverride(InSize.X);
	RootSizeBox->SetHeightOverride(InSize.Y);
}

void UCardWidget::SetCardInfo(const FCardInitParams& InitParams) const
{
	CardImage->SetBrushFromTexture(InitParams.CardDefinition->CardTexture);

	const FLinearColor& CardTypeColor = InitParams.CardViewData->GetCardTypeColor(InitParams.CardDefinition->CardTypeTag);
	TypeFrameImage->SetColorAndOpacity(CardTypeColor);
}

void UCardWidget::SetViewDetail(const FViewDetailData& InData) const
{
	CardImage->SetBrushResourceObject(InData.CardImage);
	TypeFrameImage->SetColorAndOpacity(InData.CardTypeColor);
	InvalidationBox->SetCanCache(false);
}
