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
	virtual void ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor) override;

	TArray<FGameplayEffectSpecHandle> MakeDamageSpecHandle(const UGameplayAbility* OwningAbility);
	void CauseDamage(const UGameplayAbility* OwningAbility, AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& DamageSpecs);

private:
	// 데미지 타입과 그 속성 데미지를 정의하는 변수입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TMap<FGameplayTag, FScalableFloat> DamageTypes;
};
