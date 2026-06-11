// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectApplier.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void UGameplayEffectApplier::MakeEffectContextHandle(const UGameplayAbility* OwningAbility, FGameplayEffectContextHandle& OutHandle) const
{
	// MakeEffectContext 함수는 자동으로 OwnerActor를 Instigator로, AvatarActor를 EffectCauser로 할당합니다.
	if (const UAbilitySystemComponent* OwningASC = OwningAbility->GetAbilitySystemComponentFromActorInfo())
	{
		OutHandle.Clear();
		OutHandle = OwningASC->MakeEffectContext();
		OutHandle.SetAbility(OwningAbility);
	}
}

bool UGameplayEffectApplier::TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	UAbilitySystemComponent* ASC = OwningAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}

	if (InContextHandle.IsValid())
	{
		return TryMakeSpecHandles(ASC, InContextHandle, OutSpecHandles);
	}
	return false;
}

TSubclassOf<UGameplayEffect> UGameplayEffectApplier::GetEffectClass() const
{
	return EffectClass;
}

const FGameplayTag& UGameplayEffectApplier::GetEffectApplierTag() const
{
	return EffectApplierTag;
}

TSubclassOf<UGameplayEffect> UGameplayEffectApplier::GetSourcePreviewEffectClass() const
{
	return nullptr;
}

bool UGameplayEffectApplier::TryMakeSpecHandlesForSourcePreview(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	return false;
}

int32 UGameplayEffectApplier::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	return 0;
}
