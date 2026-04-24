// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectApplier_DamageWithConsumeAllCost.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

UEffectApplier_DamageWithConsumeAllCost::UEffectApplier_DamageWithConsumeAllCost()
{
	EffectApplierTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EffectApplier.Damage.WithConsumeAllCost")));
}

bool UEffectApplier_DamageWithConsumeAllCost::TryMakeSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview) const
{
	OutSpecHandles.Reserve(DamageValues.Num() * SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute()));
	if (!bPreview)
	{
		while (SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute()) >= 1.f)
		{
			const float PrevCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
			ApplyCost(SourceASC);
			const float NewCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
		
			// 매 반복 시마다 반드시 감소해야 하므로, 이전 Cost와 현재 Cost가 일치할 경우 분기를 빠져나갑니다.
			if (PrevCost == NewCost)
			{
				break;
			}
		
			Super::TryMakeSpecHandles(SourceASC, InContextHandle, OutSpecHandles);
		}
	}
	else
	{
		const int32 StartCost = FMath::FloorToInt(SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute()));
		for (int32 Index = 0; Index < StartCost; Index++)
		{
			Super::TryMakeSpecHandles(SourceASC, InContextHandle, OutSpecHandles);
		}
	}
	return !OutSpecHandles.IsEmpty();
}

void UEffectApplier_DamageWithConsumeAllCost::ApplyCost(UAbilitySystemComponent* SourceASC) const
{
	if (!SourceASC)
	{
		return;
	}

	check(CostEffectClass);
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, FGameplayEffectContextHandle());
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

int32 UEffectApplier_DamageWithConsumeAllCost::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	float AllDamage = 0.f;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		AllDamage += Pair.Value.GetValueAtLevel(InLevel);
	}
	// Preview와 흡사한 상황으로, 설명에 텍스트를 표시하기 위한 값을 가져가는 상황이기 때문에 Cost와 곱해서 반환합니다.
	return AllDamage * (OwnerASC ? OwnerASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute()) : 1.f);
}

TSubclassOf<UGameplayEffect> UEffectApplier_DamageWithConsumeAllCost::GetSourcePreviewEffectClass() const
{
	return CostEffectClass;
}

bool UEffectApplier_DamageWithConsumeAllCost::TryMakeSpecHandlesForSourcePreview(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	if (!SourceASC)
	{
		return false;
	}
	
	const int32 CurrentCost = SourceASC->GetNumericAttribute(ULetheAttributeSet::GetCostAttribute());
	OutSpecHandles.Reserve(CurrentCost);
	for (int32 Index = 0; Index < CurrentCost; ++Index)
	{
		FGameplayEffectSpecHandle CostSpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, InContextHandle);
		OutSpecHandles.Emplace(CostSpecHandle);
	}
	return !OutSpecHandles.IsEmpty();
}
