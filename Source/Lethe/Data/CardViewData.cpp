// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardViewData.h"

FCardViewInfo* UCardViewData::FindCardInfoByTag(const FGameplayTag& InAbilityTag)
{
	return CardViewData.Find(InAbilityTag);
}

FVector2D UCardViewData::GetCardSize() const
{
	return CardSize;
}

float UCardViewData::GetCardHighlightScale() const
{
	return CardHighlightScale;
}
