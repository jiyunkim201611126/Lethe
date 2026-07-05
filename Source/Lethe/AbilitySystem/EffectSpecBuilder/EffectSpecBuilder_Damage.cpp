// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectSpecBuilder_Damage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

bool FEffectSpecBuilder_Damage::TryBuildEffectSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview) const
{
	if (!SourceASC)
	{
		return false;
	}
	
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		if (Pair.Value.IsValid())
		{
			const float ScaledDamage = Pair.Value.GetValueAtLevel(InContextHandle.GetAbilityLevel());
		
			// 할당받은 DamageEffectClass를 기반으로 GameplayEffectSpec을 생성합니다.
			FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, InContextHandle);
		
			// Spec 안에 SetByCallerMagnitudes라는 이름의 TMap이 있으며, 거기에 Tag를 키, Damage를 밸류로 값을 추가하는 함수입니다.
			// 이 값은 GetSetByCallerMagnitude로 꺼내올 수 있습니다.
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);

			OutSpecHandles.Add(DamageSpecHandle);
		}
	}
	return !OutSpecHandles.IsEmpty();
}

int32 FEffectSpecBuilder_Damage::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	float AllDamage = 0.f;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		AllDamage += Pair.Value.GetValueAtLevel(InLevel);
	}
	return AllDamage;
}
