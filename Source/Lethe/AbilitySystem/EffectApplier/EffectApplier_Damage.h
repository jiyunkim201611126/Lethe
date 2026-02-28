// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectApplier.h"
#include "ScalableFloat.h"
#include "EffectApplier_Damage.generated.h"

UCLASS()
class LETHE_API UEffectApplier_Damage : public UGameplayEffectApplier
{
	GENERATED_BODY()

public:
	//~ Begin UGameplayEffectApplier Interface
	virtual void ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor) override;
	virtual bool TryMakeSpecHandles(const UAbilitySystemComponent* SourceASC, const UGameplayAbility* OwningAbility, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bIsPreview = false) const override;
	virtual FText GetDescriptionText(const int32 InLevel) const override;
	//~ End of UGameplayEffectApplier Interface
	
	void CauseDamage(const UGameplayAbility* OwningAbility, AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& DamageSpecs);
	FText GetDamageText(const int32 InLevel, const FGameplayTag& InDamageTag) const;

protected:
	// 데미지 타입과 그 속성 데미지를 정의하는 변수입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TMap<FGameplayTag, FScalableFloat> DamageValues;
};
