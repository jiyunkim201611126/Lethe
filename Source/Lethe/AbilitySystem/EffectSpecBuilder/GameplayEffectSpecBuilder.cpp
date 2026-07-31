// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectSpecBuilder.h"

bool FGameplayEffectSpecBuilder::TryBuildSourceEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	return false;
}

bool FGameplayEffectSpecBuilder::TryBuildTargetEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	return false;
}

const FGameplayTag& FGameplayEffectSpecBuilder::GetEffectSpecBuilderTag() const
{
	return EffectSpecBuilderTag;
}

void FGameplayEffectSpecBuilder::GetDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, FText& OutDescription) const
{
}

int32 FGameplayEffectSpecBuilder::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	return 0;
}
