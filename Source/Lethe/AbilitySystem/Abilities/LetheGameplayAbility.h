// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/Data/Stage/TileData.h"
#include "LetheGameplayAbility.generated.h"

class UGameplayEffectApplier;

UENUM(BlueprintType)
enum class ENoiseStartTile : uint8
{
	StandingTile,
	TargetTile,
};

USTRUCT(BlueprintType)
struct FNoisePolicy
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	ENoiseStartTile NoiseStartTile;

	UPROPERTY(EditDefaultsOnly)
	FBFSRange NoiseRange;
};

UCLASS()
class LETHE_API ULetheGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	FBFSRange GetAbilityRange() const;

protected:
	UFUNCTION(BlueprintCallable)
	void ActivateNoise(const ATile* StandingTile, const ATile* TargetTile);

protected:
	// Ability 범위입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	FBFSRange AbilityRange;

	// Noise 정책입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	TArray<FNoisePolicy> NoisePolicies;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
