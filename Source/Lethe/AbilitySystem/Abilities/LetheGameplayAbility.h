// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/Data/Stage/TileData.h"
#include "LetheGameplayAbility.generated.h"

class UGameplayEffectApplier;

UCLASS()
class LETHE_API ULetheGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	FAbilityRange GetAbilityRange() const;

protected:
	// Ability 범위입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	FAbilityRange AbilityRange;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
