// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectSpecBuilder_DamageWithConsumeAllCost.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/LetheLog.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"

bool FEffectSpecBuilder_DamageWithConsumeAllCost::TryBuildEffectSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview) const
{
	if (!SourceASC)
	{
		return false;
	}

	const AActor* SourceActor = SourceASC->GetAvatarActor();
	if (!SourceActor || !SourceActor->Implements<UPlayerCharacterInterface>())
	{
		return false;
	}
	
	OutSpecHandles.Reserve(DamageValues.Num() * FMath::FloorToInt(SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute())));
	if (!bPreview)
	{
		while (SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute()) >= 1.f)
		{
			const float PrevCost = SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute());
			ApplyCost(SourceASC);
			const float NewCost = SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute());
			
			// 매 반복 시마다 반드시 감소해야 하므로, 이전 Cost와 현재 Cost가 일치할 경우 분기를 빠져나갑니다.
			if (PrevCost == NewCost)
			{
				LETHE_LOG(LogAbility, Error, "%s를 사용했으나, 코스트가 소모되지 않았습니다.", *StaticStruct()->GetName());
				break;
			}
			
			Super::TryBuildEffectSpecHandles(SourceASC, InContextHandle, OutSpecHandles);
		}
	}
	else
	{
		const int32 StartCost = FMath::FloorToInt(SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute()));
		for (int32 Index = 0; Index < StartCost; Index++)
		{
			Super::TryBuildEffectSpecHandles(SourceASC, InContextHandle, OutSpecHandles);
		}
	}
	return !OutSpecHandles.IsEmpty();
}

void FEffectSpecBuilder_DamageWithConsumeAllCost::ApplyCost(UAbilitySystemComponent* SourceASC) const
{
	if (!SourceASC)
	{
		return;
	}

	// Cost 개념은 플레이어 캐릭터에게만 존재합니다.
	if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(SourceASC->GetAvatarActor()))
	{
		if (CombatInterface->GetTeamSide() != ETeamSide::Player)
		{
			return;
		}
	}

	check(CostEffectClass);
	const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, FGameplayEffectContextHandle());
	if (SpecHandle.IsValid())
	{
		SourceASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

int32 FEffectSpecBuilder_DamageWithConsumeAllCost::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	float AllDamage = 0.f;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		AllDamage += Pair.Value.GetValueAtLevel(InLevel);
	}
	// Preview와 흡사한 상황으로, 설명에 텍스트를 표시하기 위한 값을 가져가는 상황이기 때문에 Cost와 곱해서 반환합니다.
	return AllDamage * (OwnerASC ? OwnerASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute()) : 1.f);
}

TSubclassOf<UGameplayEffect> FEffectSpecBuilder_DamageWithConsumeAllCost::GetSourcePreviewEffectClass() const
{
	return CostEffectClass;
}

bool FEffectSpecBuilder_DamageWithConsumeAllCost::TryBuildSourcePreviewSpecHandles(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	if (!SourceASC)
	{
		return false;
	}
	
	const int32 CurrentCost = SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute());
	OutSpecHandles.Reserve(CurrentCost);
	for (int32 Index = 0; Index < CurrentCost; ++Index)
	{
		FGameplayEffectSpecHandle CostSpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, InContextHandle);
		OutSpecHandles.Add(CostSpecHandle);
	}
	return !OutSpecHandles.IsEmpty();
}
