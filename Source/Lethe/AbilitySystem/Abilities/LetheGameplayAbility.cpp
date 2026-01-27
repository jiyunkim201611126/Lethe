// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

void ULetheGameplayAbility::ApplyAllEffects(AActor* TargetActor)
{
	for (auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			FGameplayEffectApplier& EffectApplier = InstancedApplier.GetMutable();
			if (TargetActor)
			{
				EffectApplier.ApplyEffect(this, TargetActor);
			}
		}
	}
}

FText ULetheGameplayAbility::GetCardDescription(const int32 InLevel) const
{
	TArray<FText> ResultTexts;
	for (const auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			const FGameplayEffectApplier& EffectApplier = InstancedApplier.Get();
			ResultTexts.Emplace(EffectApplier.GetDescriptionText(InLevel));
		}
	}

	return FText::Join(FText::FromString(TEXT(" ")), ResultTexts);
}

FGameplayEffectContextHandle ULetheGameplayAbility::GetContextHandle(const int32 ApplierIndex) const
{
	if (EffectAppliers.IsValidIndex(ApplierIndex))
	{
		const auto& InstancedApplier = EffectAppliers[ApplierIndex];
		if (InstancedApplier.IsValid())
		{
			const FGameplayEffectApplier& EffectApplier = InstancedApplier.Get();
			return EffectApplier.GetEffectContextHandle();
		}
	}
	return FGameplayEffectContextHandle();
}

void ULetheGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (TriggerEventData && TriggerEventData->Target)
	{
		ApplyAllEffects(const_cast<AActor*>(TriggerEventData->Target.Get()));
	}
}

void ULetheGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	for (auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			FGameplayEffectApplier& EffectApplier = InstancedApplier.GetMutable();
			EffectApplier.CancelAbility();
		}
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void ULetheGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	for (auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			FGameplayEffectApplier& EffectApplier = InstancedApplier.GetMutable();
			EffectApplier.EndAbility();
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


#if WITH_EDITOR
#include "Lethe/Manager/LetheGameplayTags.h"

void ULetheGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().CharacterState_Dead);
	}
}
#endif
