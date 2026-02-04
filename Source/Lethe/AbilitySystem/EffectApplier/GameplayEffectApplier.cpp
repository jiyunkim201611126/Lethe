// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectApplier.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void FGameplayEffectApplier::ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor)
{
}

void FGameplayEffectApplier::CancelAbility()
{
	EffectContextHandle.Clear();
}

void FGameplayEffectApplier::EndAbility()
{
	EffectContextHandle.Clear();
}

void FGameplayEffectApplier::MakeEffectContextHandle(const UGameplayAbility* OwningAbility)
{
	// EffectContext를 생성 및 할당합니다.
	// MakeEffectContext 함수는 자동으로 OwnerActor를 Instigator로, AvatarActor를 EffectCauser로 할당합니다.
	if (const UAbilitySystemComponent* AbilitySystemComponent = OwningAbility->GetAbilitySystemComponentFromActorInfo())
	{
		EffectContextHandle.Clear();
		EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
		EffectContextHandle.SetAbility(OwningAbility);
	}
}

FGameplayEffectContextHandle FGameplayEffectApplier::GetEffectContextHandle() const
{
	return EffectContextHandle;
}

FText FGameplayEffectApplier::GetDescriptionText(const int32 InLevel) const
{
	return FText();
}

TSubclassOf<UGameplayEffect> FGameplayEffectApplier::GetEffectClass() const
{
	return EffectClass;
}
