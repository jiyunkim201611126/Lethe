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

	// 어떤 Ability든 사망 시엔 발동할 수 없습니다.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().State_Character_Dead);
	}
}
#endif
