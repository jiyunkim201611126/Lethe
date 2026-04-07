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
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	FBFSRange AbilityRange;

	/** 소음 발생 정책으로, 시작 타일과 그 범위를 지정하는 변수입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	TArray<FNoisePolicy> NoisePolicies;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
