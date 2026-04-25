// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BGMThemeDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BGMManagerSubsystem.generated.h"

class UAudioComponent;

UCLASS(Config = Game)
class LETHE_API UBGMManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void PlayBGM(const EStageType StageType, const FName TrackType);

private:
	float GetTransitionStartDelay(const float CurrentAudioTrackTime) const;
	void SetTransitionTimer(const float TransitionStartDelay);
	void StartTransition();
	void OnTransitionEnded();

private:
	UPROPERTY(Config)
	TSoftObjectPtr<UBGMThemeDataAsset> BGMDataAssetPath;

	UPROPERTY()
	TObjectPtr<UBGMThemeDataAsset> BGMDataAsset;
	
	/** 현재 재생 중인 AudioComponent입니다. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> CurrentComponent;

	/** 현재 Transition 중인 AudioComponent입니다. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> TransitionComponent;

	/** Transition 중일 때 다음으로 Transition될 AudioComponent입니다. */
	UPROPERTY()
	TObjectPtr<UAudioComponent> PendingComponent;
	
	EStageType TransitionBGMStageType = EStageType::None;
	
	EStageType PendingBGMStageType = EStageType::None;
	
	double AudioStartTime = 0.f;
	float CurrentAudioTrackLength = 0.f;

	FTimerHandle TransitionTimerHandle;
	uint8 bIsTransitioning : 1 = false;
};
