// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardViewData.h"

FCardViewInfo* UCardViewData::FindCardInfoByTag(const FGameplayTag& InAbilityTag)
{
	return CardViewData.Find(InAbilityTag);
}
