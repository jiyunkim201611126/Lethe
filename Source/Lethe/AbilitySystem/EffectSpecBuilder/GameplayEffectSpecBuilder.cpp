// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectSpecBuilder.h"

bool FGameplayEffectSpecBuilder::TryBuildEffectSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview) const
{
	return false;
}

TSubclassOf<UGameplayEffect> FGameplayEffectSpecBuilder::GetEffectClass() const
{
	return EffectClass;
}

const FGameplayTag& FGameplayEffectSpecBuilder::GetEffectSpecBuilderTag() const
{
	return EffectSpecBuilderTag;
}

TSubclassOf<UGameplayEffect> FGameplayEffectSpecBuilder::GetSourcePreviewEffectClass() const
{
	return nullptr;
}

bool FGameplayEffectSpecBuilder::TryBuildSourcePreviewSpecHandles(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	return false;
}

int32 FGameplayEffectSpecBuilder::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	return 0;
}
