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

private:
#if WITH_EDITOR
	// 생성과 동시에 자동으로 ActivationBlockedTags에 CharacterState_Dead를 추가해주는 함수입니다.
	virtual void PostInitProperties() override;
#endif
};
