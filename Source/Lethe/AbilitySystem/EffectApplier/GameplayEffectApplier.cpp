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

bool UGameplayEffectApplier::TryMakeSpecHandlesForSourcePreview(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	return false;
}

FText UGameplayEffectApplier::GetDescriptionText(const int32 InLevel) const
{
	return FText();
}

bool UGameplayEffectApplier::TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, TArray<FGameplayEffectSpecHandle>& OutSpecHandles)
{
	const UAbilitySystemComponent* ASC = OwningAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return false;
	}
	
	MakeEffectContextHandle(OwningAbility);
	TryMakeSpecHandles(ASC, EffectContextHandle, OutSpecHandles);
	
	return true;
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

TSubclassOf<UGameplayEffect> UGameplayEffectApplier::GetEffectClass() const
{
	return EffectClass;
}

FGameplayEffectContextHandle UGameplayEffectApplier::GetEffectContextHandle() const
{
	return EffectContextHandle;
}

TSubclassOf<UGameplayEffect> UGameplayEffectApplier::GetSourcePreviewEffectClass() const
{
	return nullptr;
}
