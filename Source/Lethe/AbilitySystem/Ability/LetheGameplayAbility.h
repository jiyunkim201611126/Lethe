// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/Data/Stage/TileData.h"
#include "LetheGameplayAbility.generated.h"

struct FCueDataPayload;
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
	ENoiseStartTile NoiseStartTile = ENoiseStartTile::TargetTile;

	UPROPERTY(EditDefaultsOnly)
	FBFSRange NoiseRange;
};

UCLASS()
class LETHE_API ULetheGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

protected:
	UFUNCTION(BlueprintCallable)
	void ActivateNoise(const ATile* StandingTile, const ATile* TargetTile);

	/** GameplayCue 재생 시 필요한 매개변수를 받아, Cue 용도로만 사용할 EffectContext를 만들어 Handle을 반환합니다. */
	UFUNCTION(BlueprintCallable)
	void MakeEffectContextForCue(UPARAM(ref)const FCueDataPayload& CueDataPayload, FGameplayEffectContextHandle& OutHandle);

protected:
	/** 소음 발생 정책으로, 시작 타일과 그 범위를 지정하는 변수입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Noise")
	TArray<FNoisePolicy> NoisePolicies;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
