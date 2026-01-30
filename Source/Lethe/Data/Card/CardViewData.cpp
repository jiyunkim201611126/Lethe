// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardViewData.h"

FVector2D UCardViewData::GetCardSize() const
{
	return CardSize;
}

float UCardViewData::GetCardExpandScale() const
{
	return CardExpandScale;
}

FLinearColor* UCardViewData::FindCardTypeColor(const FGameplayTag& InCardTypeTag)
{
	return CardTypeColors.Find(InCardTypeTag);
}
