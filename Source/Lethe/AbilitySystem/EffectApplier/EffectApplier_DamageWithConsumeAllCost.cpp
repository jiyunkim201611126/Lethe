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

		// 매 반복 시마다 반드시 감소해야 하므로, 이전 Cost와 현재 Cost가 일치할 경우 분기를 빠져나갑니다.
		if (PrevCost == NewCost)
		{
			break;
		}
		
		Super::ApplyEffect(OwningAbility, TargetActor);
	}
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

bool UEffectApplier_DamageWithConsumeAllCost::TryBuildPreviewSpecContexts(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FPreviewEffectSpecContext>& OutPreviewEffectSpecContexts) const
{
	if (!SourceASC || !TargetASC)
	{
		return false;
	}

	const int32 CurrentCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
	if (CurrentCost <= 0)
	{
		return false;
	}

	const bool bHasTargetPreview = BuildTargetDamagePreviewContext(SourceASC, InContextHandle, CurrentCost, OutPreviewEffectSpecContexts);
	const bool bHasSourcePreview = BuildSourceCostPreviewContext(SourceASC, InContextHandle, CurrentCost, OutPreviewEffectSpecContexts);
	return bHasTargetPreview || bHasSourcePreview;
}

bool UEffectApplier_DamageWithConsumeAllCost::BuildTargetDamagePreviewContext(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, const int32 CostCount, TArray<FPreviewEffectSpecContext>& OutPreviewEffectSpecContexts) const
{
	// 해당 EffectApplier는 Cost를 1씩 줄이면서 EffectSpec을 만들어 적용하므로, Preview에선 Cost만큼 Spec을 만들어 반환해줍니다.
	TArray<FGameplayEffectSpecHandle> TargetSpecHandles;
	TargetSpecHandles.Reserve(DamageValues.Num() * CostCount);
	for (int32 Index = 0; Index < CostCount; ++Index)
	{
		TryMakeSpecHandles(SourceASC, InContextHandle, TargetSpecHandles);
	}

	if (TargetSpecHandles.IsEmpty())
	{
		return false;
	}

	FPreviewEffectSpecContext& TargetContext = OutPreviewEffectSpecContexts.Emplace_GetRef();
	TargetContext.Target = EEffectPreviewTarget::Target;
	TargetContext.EffectClass = EffectClass;
	TargetContext.SpecHandles = MoveTemp(TargetSpecHandles);
	return true;
}

bool UEffectApplier_DamageWithConsumeAllCost::BuildSourceCostPreviewContext(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, const int32 CostCount, TArray<FPreviewEffectSpecContext>& OutPreviewSpecEffectContexts) const
{
	if (!CostEffectClass)
	{
		return false;
	}

	// 단순히 Source의 모든 Cost를 소모하는 로직을 가진 EffectApplier지만, 프레임워크에 맞춰 Spec을 생성해 반환해줍니다.
	TArray<FGameplayEffectSpecHandle> SourceSpecHandles;
	SourceSpecHandles.Reserve(CostCount);
	for (int32 Index = 0; Index < CostCount; ++Index)
	{
		FGameplayEffectSpecHandle CostSpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, InContextHandle);
		SourceSpecHandles.Emplace(CostSpecHandle);
	}

	if (SourceSpecHandles.IsEmpty())
	{
		return false;
	}

	FPreviewEffectSpecContext& SourceContext = OutPreviewSpecEffectContexts.Emplace_GetRef();
	SourceContext.Target = EEffectPreviewTarget::Source;
	SourceContext.EffectClass = CostEffectClass;
	SourceContext.SpecHandles = MoveTemp(SourceSpecHandles);
	return true;
}

int32 UEffectApplier_DamageWithConsumeAllCost::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	float AllDamage = 0.f;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		AllDamage += Pair.Value.GetValueAtLevel(InLevel);
	}
	
	// 설명에 텍스트를 표시하기 위한 값을 가져가는 상황이기 때문에 단순하게 Cost와 곱해서 반환합니다.
	return AllDamage * (OwnerASC ? OwnerASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute()) : 1.f);
}
