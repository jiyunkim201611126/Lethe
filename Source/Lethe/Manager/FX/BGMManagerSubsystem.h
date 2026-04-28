// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BGMThemeDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BGMManagerSubsystem.generated.h"

class UAudioComponent;

enum class EBGMPlaybackState : uint8
{
	Stopped,
	Playing,
	TransitionScheduled,
	Transitioning,
};

USTRUCT()
struct FBGMPlaybackSlot
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UAudioComponent> Component;

	EStageType StageType = EStageType::None;
	FName TrackType;
	float Duration = 0.f;
	float TrackStartTime = 0.f;
	double PlaybackStartTime = 0.0;

	void Reset()
	{
		Component = nullptr;
		StageType = EStageType::None;
		TrackType = NAME_None;
		Duration = 0.f;
		TrackStartTime = 0.f;
		PlaybackStartTime = 0.0;
	}
};

UCLASS(Config = Game)
class LETHE_API UBGMManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	void PlayBGM(const EStageType StageType, const FName TrackType);

private:
	bool LoadBGM(const EStageType StageType, const FName TrackType, const FBGMTheme*& OutTheme, const FBGMTracks*& OutTrack);
	void PrepareSlot(FBGMPlaybackSlot& Slot, const EStageType StageType, const FName TrackType, const FBGMTracks& Track, const float StartTime) const;
	void StopSlot(FBGMPlaybackSlot& Slot) const;
	
	void ScheduleTransition(const EStageType StageType, const FName TrackType, const FBGMTracks& Track, const float TransitionDelay, const float StartTime);
	void StartTransition();
	void FinishTransition();
	void AbortTransition();
	float GetCurrentTrackTime() const;

private:
	UPROPERTY(Config)
	TSoftObjectPtr<UBGMThemeDataAsset> BGMDataAssetPath;

	UPROPERTY()
	TObjectPtr<UBGMThemeDataAsset> BGMDataAsset;

	UPROPERTY()
	FBGMPlaybackSlot Current;

	UPROPERTY()
	FBGMPlaybackSlot Transition;

	EStageType PendingStageType = EStageType::None;
	FName PendingTrackType;

	FTimerHandle TransitionTimerHandle;
	EBGMPlaybackState PlaybackState = EBGMPlaybackState::Stopped;
};
