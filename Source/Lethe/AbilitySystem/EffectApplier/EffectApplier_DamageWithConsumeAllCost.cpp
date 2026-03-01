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
		const float PrevCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
		ApplyCost(SourceASC, OwningAbility);
		const float NewCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());

		// 매 반복 시마다 반드시 감소해야 합니다.
		check(NewCost < PrevCost)
		Super::ApplyEffect(OwningAbility, TargetActor);
	}
}

bool UEffectApplier_DamageWithConsumeAllCost::TryMakeSpecHandles(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bIsPreview) const
{
	if (!bIsPreview)
	{
		return Super::TryMakeSpecHandles(SourceASC, InContextHandle, OutSpecHandles, bIsPreview);
	}

	OutSpecHandles.Reserve(DamageValues.Num());
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		if (Pair.Value.IsValid())
		{
			/**
			 * 해당 Effect Applier는 다단히트로, 현재 Cost를 파악해 그 수만큼 GE를 반복적용합니다.
			 * 하지만 프리뷰 상황인 경우 굳이 그럴 필요까진 없으므로, Cost를 가져와 ScaledDamage에 곱해서 사용합니다.
			 */
			float ScaledDamage = Pair.Value.GetValueAtLevel(InContextHandle.GetAbilityLevel());
			ScaledDamage *= SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
			FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, InContextHandle);
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);
			OutSpecHandles.Emplace(DamageSpecHandle);
		}
	}
	return !OutSpecHandles.IsEmpty();
}

bool UEffectApplier_DamageWithConsumeAllCost::TryMakeSpecHandlesForSourcePreview(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	const int32 CurrentCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
	OutSpecHandles.Reserve(CurrentCost);
	for (int32 Index = 0; Index < CurrentCost; ++Index)
	{
		FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, InContextHandle);
		OutSpecHandles.Emplace(DamageSpecHandle);
	}
	return !OutSpecHandles.IsEmpty();
}

TSubclassOf<UGameplayEffect> UEffectApplier_DamageWithConsumeAllCost::GetSourcePreviewEffectClass() const
{
	return CostEffectClass;
}

void UEffectApplier_DamageWithConsumeAllCost::ApplyCost(UAbilitySystemComponent* SourceASC, const UGameplayAbility* OwningAbility) const
{
	if (!SourceASC || !OwningAbility)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.SetAbility(OwningAbility);

	check(CostEffectClass);
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}
