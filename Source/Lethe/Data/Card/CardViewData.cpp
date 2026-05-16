// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardViewData.h"

FLinearColor UCardViewData::GetCardTypeColor(const FGameplayTag& InCardTypeTag) const
{
	return CardTypeColors.FindRef(InCardTypeTag);
}
