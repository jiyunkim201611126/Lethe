// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectSpecBuilder_Damage.h"
#include "EffectSpecBuilder_DamageWithConsumeAllCost.generated.h"

/**
 * 모든 Cost가 소모될 때까지 Cost를 1씩 소비하며 데미지를 다단히트 형식으로 주는 EffectSpecBuilder입니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FEffectSpecBuilder_DamageWithConsumeAllCost : public FEffectSpecBuilder_Damage
{
	GENERATED_BODY()

	//~ Begin FGameplayEffectSpecBuilder Interface
	virtual bool TryBuildSourceEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const override;
	virtual bool TryBuildTargetEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const override;
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const override;
	//~ End of FGameplayEffectSpecBuilder Interface

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	TSubclassOf<UGameplayEffect> CostEffectClass;
};
