// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BGMThemeDataAsset.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BGMManagerSubsystem.generated.h"

class UAudioComponent;

enum class EBGMPlaybackState : uint8
{
	/** 아무것도 재생하지 않는 상태입니다. */
	Stopped,
	/** BGM을 재생 중인 상태입니다. */
	Playing,
	/** BGM을 재생 중이며, Transition 예약이 걸려있는 상태입니다. */
	TransitionScheduled,
	/** BGM을 2개 재생 중이며, 페이드인/아웃 중인 상태입니다. */
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
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "PlayImmediately가 true라면, Transition 규칙을 무시하고 즉시 Transition을 시작합니다."))
	void RequestPlayBGM(const EStageType StageType, const FName TrackType, const bool bPlayImmediately = false);

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

	uint8 bIsPendingImmediately : 1 = false;
	EStageType PendingStageType = EStageType::None;
	FName PendingTrackType;

	FTimerHandle TransitionTimerHandle;
	EBGMPlaybackState PlaybackState = EBGMPlaybackState::Stopped;
};
