// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectApplier_DamageWithConsumeAllCost.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void UEffectApplier_DamageWithConsumeAllCost::ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor)
{
	UAbilitySystemComponent* SourceASC = OwningAbility ? OwningAbility->GetAbilitySystemComponentFromActorInfo() : nullptr;
	if (!SourceASC)
	{
		return;
	}

	while (SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute()) >= 1.f)
	{
		ApplyCostMinusOneToSelf(SourceASC, OwningAbility);
		Super::ApplyEffect(OwningAbility, TargetActor);
	}
}

void UEffectApplier_DamageWithConsumeAllCost::ApplyCostMinusOneToSelf(UAbilitySystemComponent* SourceASC, const UGameplayAbility* OwningAbility) const
{
	if (!SourceASC || !OwningAbility)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.SetAbility(OwningAbility);

	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(CostMinusOneEffectClass, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
