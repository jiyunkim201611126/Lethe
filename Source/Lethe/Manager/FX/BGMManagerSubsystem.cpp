// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMManagerSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"

void UBGMManagerSubsystem::RequestPlayBGM(const EStageType StageType, const FName TrackType, const bool bPlayImmediately)
{
	const FBGMTheme* BGMTheme = nullptr;
	const FBGMTracks* BGMTrack = nullptr;
	if (!LoadBGM(StageType, TrackType, BGMTheme, BGMTrack))
	{
		return;
	}

	// 아무것도 재생하고 있지 않은 상태라면 Current 슬롯에 넣어 재생합니다.
	if (PlaybackState == EBGMPlaybackState::Stopped)
	{
		PrepareSlot(Current, StageType, TrackType, *BGMTrack, 0.f);
		if (Current.Component)
		{
			Current.Component->Play(0.f);
			PlaybackState = EBGMPlaybackState::Playing;
		}
		return;
	}

	// 이미 동일한 BGM을 재생하고 있는 경우 들어가는 분기입니다.
	if (Current.StageType == StageType && Current.TrackType == TrackType)
	{
		// 아직 Transition이 시작되지 않았다면 대기 상태인 모든 BGM을 취소합니다.
		if (PlaybackState == EBGMPlaybackState::TransitionScheduled)
		{
			GetGameInstance()->GetTimerManager().ClearTimer(TransitionStartTimerHandle);
			StopSlot(Transition);
			PlaybackState = EBGMPlaybackState::Playing;
			bIsPendingImmediately = false;
			PendingStageType = EStageType::None;
			PendingTrackType = NAME_None;
			return;
		}

		// 일반 재생 상태라면 요청을 무시합니다.
		if (PlaybackState == EBGMPlaybackState::Playing)
		{
			return;
		}
	}

	if (bPlayImmediately)
	{
		if (PlaybackState != EBGMPlaybackState::Transitioning)
		{
			// Transition 중이 아니라면 모든 예약 상태를 파기하고 즉시 Transition을 시작합니다.
			bIsPendingImmediately = false;
			PendingStageType = EStageType::None;
			PendingTrackType = NAME_None;
			ScheduleTransition(StageType, TrackType, *BGMTrack, 0.f, 0.f);
			return;
		}
		
		// Transition 중일 때, Transition 예약 대기 상태와 다른 BGM인 경우일 때만 예약 대기로 걸어둡니다.
		bIsPendingImmediately = true;
		if (PendingStageType != StageType || PendingTrackType != TrackType)
		{
			PendingStageType = StageType;
			PendingTrackType = TrackType;
		}
		return;
	}

	// Transition 예약이 걸려있는 상태에서, Transition 대상 BGM과 동일한 BGM 재생 요청이라면 무시합니다.
	if (PlaybackState == EBGMPlaybackState::TransitionScheduled && Transition.StageType == StageType && Transition.TrackType == TrackType)
	{
		return;
	}

	// Transition 중일 때 Transition 대상 BGM과 동일한 BGM 재생 요청이라면, Pending을 초기화합니다.
	if (PlaybackState == EBGMPlaybackState::Transitioning && Transition.StageType == StageType && Transition.TrackType == TrackType)
	{
		bIsPendingImmediately = false;
		PendingStageType = EStageType::None;
		PendingTrackType = NAME_None;
		return;
	}

	// 예약 대기 상태인 BGM과 동일한 BGM 재생 요청이라면 무시합니다.
	if (PendingStageType == StageType && PendingTrackType == TrackType)
	{
		return;
	}

	// Transition 중이라면 예약 대기 상태에 걸어놓습니다.
	if (PlaybackState == EBGMPlaybackState::Transitioning)
	{
		bIsPendingImmediately = false;
		PendingStageType = StageType;
		PendingTrackType = TrackType;
		return;
	}

	// 그 외의 경우 Transition 예약이 필요하므로 아래 로직을 수행합니다.
	float TransitionDelay = 0.f;
	float TargetTrackTime = 0.f;
	if (Current.StageType == StageType)
	{
		if (!BGMDataAsset->GetNextTransitionInfo(StageType, Current.TrackType, TrackType, GetCurrentTrackTime(), TransitionDelay, TargetTrackTime))
		{
			LETHE_LOG(LogBGMManager, Warning, "%s Stage의 %s -> %s TransitionPoint가 없습니다. 즉시 전환합니다.", *LogHelper::EnumToString(StageType), *Current.TrackType.ToString(), *TrackType.ToString());
		}
	}

	ScheduleTransition(StageType, TrackType, *BGMTrack, TransitionDelay, TargetTrackTime);
}

bool UBGMManagerSubsystem::LoadBGM(const EStageType StageType, const FName TrackType, const FBGMTheme*& OutTheme, const FBGMTracks*& OutTrack)
{
	if (!BGMDataAsset)
	{
		BGMDataAsset = BGMDataAssetPath.LoadSynchronous();
	}

	if (!BGMDataAsset)
	{
		LETHE_LOG(LogBGMManager, Error, "BGMDataAssetPath가 설정되지 않았습니다.");
		return false;
	}

	OutTheme = BGMDataAsset->GetTheme(StageType);
	if (!OutTheme)
	{
		LETHE_LOG(LogBGMManager, Error, "%s에 해당하는 Theme가 없습니다.", *LogHelper::EnumToString(StageType));
		return false;
	}

	OutTrack = BGMDataAsset->GetTrack(StageType, TrackType);
	if (!OutTrack || !OutTrack->Wave || !OutTrack->LoopMetaSound)
	{
		LETHE_LOG(LogBGMManager, Error, "%s에 해당하는 Track이 없습니다.", *TrackType.ToString());
		return false;
	}

	return true;
}

void UBGMManagerSubsystem::PrepareSlot(FBGMPlaybackSlot& Slot, const EStageType StageType, const FName TrackType, const FBGMTracks& Track, const float StartTime) const
{
	// 사운드를 생성해두기만 하고, 실제 재생은 StartTransition에서 수행합니다.
	Slot.Component = UGameplayStatics::CreateSound2D(this, Track.LoopMetaSound, 1.f, 1.f, 0.f, nullptr, true, false);
	if (!Slot.Component)
	{
		return;
	}

	Slot.StageType = StageType;
	Slot.TrackType = TrackType;
	Slot.Duration = Track.Wave->GetDuration();
	Slot.TrackStartTime = StartTime;
	Slot.PlaybackStartTime = FApp::GetCurrentTime() - StartTime;
}

void UBGMManagerSubsystem::StopSlot(FBGMPlaybackSlot& Slot) const
{
	if (Slot.Component)
	{
		Slot.Component->Deactivate();
		Slot.Component->DestroyComponent();
	}

	Slot.Reset();
}

void UBGMManagerSubsystem::ScheduleTransition(const EStageType StageType, const FName TrackType, const FBGMTracks& Track, const float TransitionDelay, const float StartTime)
{
	// 기존 Transition 예약을 파기하고 새로운 Transition을 예약합니다.
	GetGameInstance()->GetTimerManager().ClearTimer(TransitionStartTimerHandle);
	GetGameInstance()->GetTimerManager().ClearTimer(TransitionFinishTimerHandle);
	StopSlot(Transition);

	Transition.StageType = StageType;
	Transition.TrackType = TrackType;
	Transition.Duration = Track.Wave->GetDuration();
	Transition.TrackStartTime = StartTime;

	if (TransitionDelay <= 0.f)
	{
		StartTransition();
		return;
	}

	PlaybackState = EBGMPlaybackState::TransitionScheduled;
	const FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::StartTransition);
	GetGameInstance()->GetTimerManager().SetTimer(TransitionStartTimerHandle, TimerDelegate, TransitionDelay, false);
}

void UBGMManagerSubsystem::StartTransition()
{
	const FBGMTheme* BGMTheme = nullptr;
	const FBGMTracks* BGMTrack = nullptr;
	if (!LoadBGM(Transition.StageType, Transition.TrackType, BGMTheme, BGMTrack) || !Current.Component)
	{
		AbortTransition();
		return;
	}

	PrepareSlot(Transition, Transition.StageType, Transition.TrackType, *BGMTrack, Transition.TrackStartTime);
	if (!Transition.Component)
	{
		AbortTransition();
		return;
	}

	// 곡 전환을 시작합니다.
	PlaybackState = EBGMPlaybackState::Transitioning;
	if (Current.Component)
	{
		Current.Component->FadeOut(BGMTheme->FadeDuration, 0.f);
	}

	if (Transition.Component)
	{
		Transition.Component->FadeIn(BGMTheme->FadeDuration, 1.f, Transition.TrackStartTime);
	}

	const FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinishTransition);
	GetGameInstance()->GetTimerManager().SetTimer(TransitionFinishTimerHandle, FinishDelegate, BGMTheme->FadeDuration, false);
}

void UBGMManagerSubsystem::FinishTransition()
{
	StopSlot(Current);

	Current = Transition;
	Transition.Reset();

	PlaybackState = EBGMPlaybackState::Playing;

	if (PendingTrackType != NAME_None)
	{
		const bool bImmediately = bIsPendingImmediately;
		const EStageType RequestedStageType = PendingStageType;
		const FName RequestedTrackType = PendingTrackType;
		bIsPendingImmediately = false;
		PendingStageType = EStageType::None;
		PendingTrackType = NAME_None;
		RequestPlayBGM(RequestedStageType, RequestedTrackType, bImmediately);
	}
}

void UBGMManagerSubsystem::AbortTransition()
{
	GetGameInstance()->GetTimerManager().ClearTimer(TransitionStartTimerHandle);
	GetGameInstance()->GetTimerManager().ClearTimer(TransitionFinishTimerHandle);
	StopSlot(Transition);

	PlaybackState = Current.Component ? EBGMPlaybackState::Playing : EBGMPlaybackState::Stopped;

	if (PendingTrackType != NAME_None)
	{
		const bool bImmediately = bIsPendingImmediately;
		const EStageType RequestedStageType = PendingStageType;
		const FName RequestedTrackType = PendingTrackType;
		bIsPendingImmediately = false;
		PendingStageType = EStageType::None;
		PendingTrackType = NAME_None;
		RequestPlayBGM(RequestedStageType, RequestedTrackType, bImmediately);
	}
}

float UBGMManagerSubsystem::GetCurrentTrackTime() const
{
	if (Current.Duration <= 0.f || Current.Duration == INDEFINITELY_LOOPING_DURATION)
	{
		LETHE_LOG(LogBGMManager, Error, "Stage: %s, Track: %s의 Duration이 유효하지 않습니다. Duration: %f", *LogHelper::EnumToString(Current.StageType), *Current.TrackType.ToString(), Current.Duration);
		return 0.f;
	}
	return FMath::Fmod(FApp::GetCurrentTime() - Current.PlaybackStartTime, Current.Duration);
}
