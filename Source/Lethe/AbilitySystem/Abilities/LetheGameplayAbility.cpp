// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

void ULetheGameplayAbility::ApplyAllEffects(AActor* TargetActor)
{
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier && TargetActor)
		{
			EffectApplier->ApplyEffect(this, TargetActor);
		}
	}
}

FText ULetheGameplayAbility::GetCardDescription(const int32 InLevel) const
{
	TArray<FText> ResultTexts;
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		ResultTexts.Emplace(EffectApplier->GetDescriptionText(InLevel));
	}

	return FText::Join(FText::FromString(TEXT(" ")), ResultTexts);
}

FGameplayEffectContextHandle ULetheGameplayAbility::GetContextHandle(const TSubclassOf<UGameplayEffectApplier>& ApplierClass) const
{
	for (const auto EffectApplier : EffectAppliers)
	{
		if (EffectApplier && EffectApplier->GetClass() == ApplierClass)
		{
			return EffectApplier->GetEffectContextHandle();
		}
	}
	return FGameplayEffectContextHandle();
}

void ULetheGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	for (const auto EffectApplier : EffectAppliers)
	{
		EffectApplier->CancelAbility();
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void ULetheGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	for (const auto EffectApplier : EffectAppliers)
	{
		EffectApplier->EndAbility();
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
