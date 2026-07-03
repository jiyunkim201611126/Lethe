// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectApplier.h"
#include "ScalableFloat.h"
#include "EffectApplier_Damage.generated.h"

USTRUCT(BlueprintType)
struct LETHE_API FEffectApplier_Damage : public FGameplayEffectApplier
{
	GENERATED_BODY()

	//~ Begin FGameplayEffectApplier Interface
	virtual void ApplyEffect(const UGameplayAbility* OwningAbility, AActor* TargetActor) const override;
	virtual bool TryPrepareSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview = false) const override;

	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const override;
	//~ End of FGameplayEffectApplier Interface
	
	void CauseDamage(const UGameplayAbility* OwningAbility, AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& DamageSpecs) const;

protected:
	/** 데미지 타입과 그 값을 정의하는 변수입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TMap<FGameplayTag, FScalableFloat> DamageValues;
};
