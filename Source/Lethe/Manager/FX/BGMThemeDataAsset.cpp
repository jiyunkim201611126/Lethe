// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMThemeDataAsset.h"

#include "Sound/SoundWave.h"

const FBGMTheme* UBGMThemeDataAsset::GetTheme(const EStageType StageType) const
{
	return BGMThemes.Find(StageType);
}

const FBGMTracks* UBGMThemeDataAsset::GetTrack(const EStageType StageType, const FName TrackType) const
{
	const FBGMTheme* BGMTheme = GetTheme(StageType);
	if (!BGMTheme)
	{
		return nullptr;
	}

	return BGMTheme->Tracks.Find(TrackType);
}

bool UBGMThemeDataAsset::GetNextTransitionInfo(const EStageType StageType, const FName FromTrackType, const FName ToTrackType, const float CurrentTrackTime, float& OutDelay, float& OutTargetTrackTime) const
{
	const FBGMTheme* BGMTheme = GetTheme(StageType);
	const FBGMTracks* FromTrack = GetTrack(StageType, FromTrackType);
	if (!BGMTheme || !FromTrack || !FromTrack->Wave)
	{
		return false;
	}

	bool bFound = false;
	float BestDelay = MAX_FLT;
	float BestTargetTrackTime = 0.f;

	for (const FBGMTransitionPoint& TransitionPoint : BGMTheme->TransitionPoints)
	{
		float FromTrackTransitionTime;
		float ToTrackTransitionTime;

		if (TransitionPoint.TrackA == FromTrackType && TransitionPoint.TrackB == ToTrackType)
		{
			FromTrackTransitionTime = TransitionPoint.TrackATime;
			ToTrackTransitionTime = TransitionPoint.TrackBTime;
		}
		else if (TransitionPoint.TrackB == FromTrackType && TransitionPoint.TrackA == ToTrackType)
		{
			FromTrackTransitionTime = TransitionPoint.TrackBTime;
			ToTrackTransitionTime = TransitionPoint.TrackATime;
		}
		else
		{
			continue;
		}

		const float FromTrackDuration = FromTrack->Wave->GetDuration();
		const float Delay = CurrentTrackTime <= FromTrackTransitionTime
			? FromTrackTransitionTime - CurrentTrackTime
			: FromTrackDuration - CurrentTrackTime + FromTrackTransitionTime;

		if (Delay < BestDelay)
		{
			bFound = true;
			BestDelay = Delay;
			BestTargetTrackTime = ToTrackTransitionTime;
		}
	}

	if (!bFound)
	{
		return false;
	}

	OutDelay = BestDelay;
	OutTargetTrackTime = BestTargetTrackTime;
	return true;
}
