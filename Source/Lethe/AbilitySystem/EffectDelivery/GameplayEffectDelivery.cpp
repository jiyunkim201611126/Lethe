// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectDelivery.h"

bool FEffectDeliveryContext::IsValid() const
{
	return !EffectSpecHandles.IsEmpty() && OwnerAbility.IsValid() && SourceASC.IsValid() && TargetASC.IsValid();
}

void FGameplayEffectDelivery::StartDelivery(const FEffectDeliveryContext& Context) const
{
}
