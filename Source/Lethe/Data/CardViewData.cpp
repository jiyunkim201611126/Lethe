// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardViewData.h"

FCardSelfViewInfo* UCardViewData::FindCardSelfViewInfoByTag(const FGameplayTag& InCardTag)
{
	return CardSelfViewData.Find(InCardTag);
}

FCardOwnerViewInfo* UCardViewData::FindCardOwnerViewInfoByTag(const FGameplayTag& InCharacterTag)
{
	return CardOwnerViewData.Find(InCharacterTag);
}

FVector2D UCardViewData::GetCardSize() const
{
	return CardSize;
}

float UCardViewData::GetCardHighlightScale() const
{
	return CardHighlightScale;
}

FLinearColor* UCardViewData::FindCardTypeColor(const FGameplayTag& InCardTypeTag)
{
	return CardTypeColors.Find(InCardTypeTag);
}
