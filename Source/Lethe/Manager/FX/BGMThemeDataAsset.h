// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Lethe/Data/Stage/StageData.h"
#include "BGMThemeDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FBGMTracks
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> Wave;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> LoopMetaSound;
};

USTRUCT(BlueprintType)
struct FBGMTransitionPoint
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName TrackA;

	UPROPERTY(EditDefaultsOnly)
	float TrackATime = 0.f;

	UPROPERTY(EditDefaultsOnly)
	FName TrackB;

	UPROPERTY(EditDefaultsOnly)
	float TrackBTime = 0.f;
};

USTRUCT(BlueprintType)
struct FBGMTheme
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TMap<FName, TObjectPtr<USoundBase>> Tracks;

	UPROPERTY(EditDefaultsOnly)
	TArray<FBGMTransitionPoint> TransitionPoints;

	UPROPERTY(EditDefaultsOnly)
	float FadeDuration = 2.f;
};

UCLASS()
class LETHE_API UBGMThemeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	const FBGMTheme* GetTheme(const EStageType StageType) const;
	USoundBase* GetTrack(const EStageType StageType, const FName TrackType) const;
	bool GetNextTransitionInfo(const EStageType StageType, const FName FromTrackType, const FName ToTrackType, const float CurrentTrackTime, float& OutDelay, float& OutTargetTrackTime) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TMap<EStageType, FBGMTheme> BGMThemes;
};
