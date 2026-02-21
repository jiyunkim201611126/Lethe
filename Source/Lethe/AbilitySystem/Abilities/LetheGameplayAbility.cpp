// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

#include "Lethe/Manager/LetheGameplayTags.h"

FAbilityRange ULetheGameplayAbility::GetAbilityRange() const
{
	return AbilityRange;
}

#if WITH_EDITOR
void ULetheGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().CharacterState_Dead);
	}
}
#endif
