// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectApplier.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void FGameplayEffectApplier::ApplyEffect(const UGameplayAbility* OwningAbility, AActor* TargetActor) const
{
}

bool FGameplayEffectApplier::TryPrepareSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview) const
{
	return false;
}

void FGameplayEffectApplier::MakeEffectContextHandle(const UGameplayAbility* OwningAbility, FGameplayEffectContextHandle& OutHandle) const
{
	// MakeEffectContext 함수는 자동으로 OwnerActor를 Instigator로, AvatarActor를 EffectCauser로 할당합니다.
	if (const UAbilitySystemComponent* OwningASC = OwningAbility->GetAbilitySystemComponentFromActorInfo())
	{
		OutHandle.Clear();
		OutHandle = OwningASC->MakeEffectContext();
		OutHandle.SetAbility(OwningAbility);
	}
}

bool FGameplayEffectApplier::TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	UAbilitySystemComponent* ASC = OwningAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	if (InContextHandle.IsValid())
	{
		return TryPrepareSpecHandles(ASC, InContextHandle, OutSpecHandles);
	}
	return false;
}

TSubclassOf<UGameplayEffect> FGameplayEffectApplier::GetEffectClass() const
{
	return EffectClass;
}

const FGameplayTag& FGameplayEffectApplier::GetEffectApplierTag() const
{
	return EffectApplierTag;
}

TSubclassOf<UGameplayEffect> FGameplayEffectApplier::GetSourcePreviewEffectClass() const
{
	return nullptr;
}

bool FGameplayEffectApplier::TryMakeSourcePreviewSpecHandles(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	return false;
}

int32 FGameplayEffectApplier::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	return 0;
}
