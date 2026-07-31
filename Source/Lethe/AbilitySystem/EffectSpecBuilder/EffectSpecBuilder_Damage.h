// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectSpecBuilder.h"
#include "ScalableFloat.h"
#include "EffectSpecBuilder_Damage.generated.h"

USTRUCT(BlueprintType)
struct LETHE_API FEffectSpecBuilder_Damage : public FGameplayEffectSpecBuilder
{
	GENERATED_BODY()

public:
	virtual bool TryBuildTargetEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const override;
	virtual void GetDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, FText& OutDescription) const override;
	
protected:
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const override;

protected:
	/** 데미지 타입과 그 값을 정의하는 변수입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TMap<FGameplayTag, FScalableFloat> DamageValues;
};
