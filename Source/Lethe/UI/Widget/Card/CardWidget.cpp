// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "Components/Image.h"
#include "Lethe/Data/CardViewData.h"

void UCardWidget::UpdateCardView(const FCardViewInfo* InCardInfo) const
{
	if (InCardInfo)
	{
		CardImage->SetBrushFromTexture(InCardInfo->CardTexture);
	}
}
