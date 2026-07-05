// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectDelivery_Immediately.h"

#include "AbilitySystemComponent.h"

void FEffectDelivery_Immediately::StartDelivery(const FEffectDeliveryContext& Context) const
{
	if (!Context.IsValid())
	{
		return;
	}
	
	for (const FGameplayEffectSpecHandle& SpecHandle : Context.EffectSpecHandles)
	{
		if (SpecHandle.IsValid())
		{
			Context.SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), Context.TargetASC.Get());
		}
	}
}
