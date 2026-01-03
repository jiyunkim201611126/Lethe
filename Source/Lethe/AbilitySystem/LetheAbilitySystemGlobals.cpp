// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemGlobals.h"

#include "Lethe/LetheAbilityTypes.h"

FGameplayEffectContext* ULetheAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FLetheGameplayEffectContext();
}
