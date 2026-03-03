// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectApplier_Damage.h"
#include "EffectApplier_DamageWithConsumeAllCost.generated.h"

/**
 * 모든 Cost가 소모될 때까지 Cost를 1씩 소비하며 데미지를 다단히트 형식으로 주는 EA입니다.
 */
UCLASS()
class LETHE_API UEffectApplier_DamageWithConsumeAllCost : public UEffectApplier_Damage
{
	GENERATED_BODY()

public:
	//~ Begin FGameplayEffectApplier Interface
	virtual void ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor) override;

	virtual bool TryBuildPreviewSpecContexts(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FPreviewEffectSpecContext>& OutPreviewEffectSpecContexts) const override;
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const override;
	//~ End of FGameplayEffectApplier Interface

private:
	void ApplyCost(UAbilitySystemComponent* SourceASC, const UGameplayAbility* OwningAbility) const;
	
	bool BuildTargetDamagePreviewContext(const UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, const int32 CostCount, TArray<FPreviewEffectSpecContext>& OutPreviewEffectSpecContexts) const;
	bool BuildSourceCostPreviewContext(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, const int32 CostCount, TArray<FPreviewEffectSpecContext>& OutPreviewSpecEffectContexts) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Cost")
	TSubclassOf<UGameplayEffect> CostEffectClass;
};
