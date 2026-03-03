// Copyright JETBLU, Inc. All Rights Reserved.

#include "GameplayEffectApplier.h"

#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

void UGameplayEffectApplier::CancelAbility()
{
	EffectContextHandle.Clear();
}

void UGameplayEffectApplier::EndAbility()
{
	EffectContextHandle.Clear();
}

bool UGameplayEffectApplier::TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, TArray<FGameplayEffectSpecHandle>& OutSpecHandles)
{
	const UAbilitySystemComponent* ASC = OwningAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}
	
	MakeEffectContextHandle(OwningAbility);
	return TryMakeSpecHandles(ASC, EffectContextHandle, OutSpecHandles);
}

void UGameplayEffectApplier::MakeEffectContextHandle(const UGameplayAbility* OwningAbility)
{
	// EffectContext를 생성 및 할당합니다.
	// MakeEffectContext 함수는 자동으로 OwnerActor를 Instigator로, AvatarActor를 EffectCauser로 할당합니다.
	if (const UAbilitySystemComponent* OwningASC = OwningAbility->GetAbilitySystemComponentFromActorInfo())
	{
		EffectContextHandle.Clear();
		EffectContextHandle = OwningASC->MakeEffectContext();
		EffectContextHandle.SetAbility(OwningAbility);
	}
}

FGameplayEffectContextHandle UGameplayEffectApplier::GetEffectContextHandle() const
{
	return EffectContextHandle;
}

bool UGameplayEffectApplier::TryBuildPreviewSpecContexts(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FPreviewEffectSpecContext>& OutPreviewEffectSpecContexts) const
{
	if (!SourceASC || !TargetASC)
	{
		return false;
	}

	TArray<FGameplayEffectSpecHandle> TargetSpecHandles;
	if (TryMakeSpecHandles(SourceASC, InContextHandle, TargetSpecHandles) && EffectClass)
	{
		FPreviewEffectSpecContext& TargetContext = OutPreviewEffectSpecContexts.Emplace_GetRef();
		TargetContext.Target = EEffectPreviewTarget::Target;
		TargetContext.EffectClass = EffectClass;
		TargetContext.SpecHandles = MoveTemp(TargetSpecHandles);
	}

	return !OutPreviewEffectSpecContexts.IsEmpty();
}

int32 UGameplayEffectApplier::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	return 0;
}
