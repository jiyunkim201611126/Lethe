// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMManagerSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"

void UBGMManagerSubsystem::PlayBGM(const EStageType StageType, const FName TrackType)
{
	const FBGMTheme* BGMTheme = nullptr;
	USoundBase* BGMSound = nullptr;
	if (!LoadBGM(StageType, TrackType, BGMTheme, BGMSound))
	{
		return;
	}

	// 아무것도 재생하고 있지 않은 상태라면 Current 슬롯에 넣어 재생합니다.
	if (PlaybackState == EBGMPlaybackState::Stopped)
	{
		PlaySlot(Current, StageType, TrackType, BGMSound, 0.f, false);
		PlaybackState = EBGMPlaybackState::Playing;
		return;
	}

	// 이미 동일한 BGM을 재생하고 있는 경우 들어가는 분기입니다.
	if (Current.StageType == StageType && Current.TrackType == TrackType)
	{
		// 아직 Transition이 시작되지 않았다면 대기 상태인 모든 BGM을 취소합니다.
		if (PlaybackState == EBGMPlaybackState::TransitionScheduled)
		{
			GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
			StopSlot(Transition);
			PlaybackState = EBGMPlaybackState::Playing;
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

	// Transition 중이거나 예약이 걸려있는 상태에서, Transition 대상 BGM과 동일한 BGM 재생 요청이라면 무시합니다.
	if ((PlaybackState == EBGMPlaybackState::TransitionScheduled || PlaybackState == EBGMPlaybackState::Transitioning)
		&& Transition.StageType == StageType && Transition.TrackType == TrackType)
	{
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

	ScheduleTransition(StageType, TrackType, BGMSound, TransitionDelay, TargetTrackTime);
}

bool UBGMManagerSubsystem::LoadBGM(const EStageType StageType, const FName TrackType, const FBGMTheme*& OutTheme, USoundBase*& OutSound)
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

	OutSound = BGMDataAsset->GetTrack(StageType, TrackType);
	if (!OutSound)
	{
		LETHE_LOG(LogBGMManager, Error, "%s에 해당하는 Track이 없습니다.", *TrackType.ToString());
		return false;
	}

	return true;
}

void UBGMManagerSubsystem::ScheduleTransition(const EStageType StageType, const FName TrackType, const USoundBase* Sound, const float TransitionDelay, const float StartTime)
{
	// 기존 Transition 예약을 파기하고 새로운 Transition을 예약합니다.
	GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
	StopSlot(Transition);

	Transition.StageType = StageType;
	Transition.TrackType = TrackType;
	Transition.Duration = Sound->GetDuration();
	Transition.StartTime = FMath::Fmod(StartTime, Transition.Duration);

	if (TransitionDelay <= 0.f)
	{
		StartTransition();
		return;
	}

	PlaybackState = EBGMPlaybackState::TransitionScheduled;
	const FTimerDelegate TimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::StartTransition);
	GetWorld()->GetTimerManager().SetTimer(TransitionTimerHandle, TimerDelegate, TransitionDelay, false);
}

void UBGMManagerSubsystem::StartTransition()
{
	const FBGMTheme* BGMTheme = nullptr;
	USoundBase* BGMSound = nullptr;
	if (!LoadBGM(Transition.StageType, Transition.TrackType, BGMTheme, BGMSound) || !Current.Component)
	{
		return;
	}

	PlaySlot(Transition, Transition.StageType, Transition.TrackType, BGMSound, Transition.StartTime, true);
	if (!Transition.Component)
	{
		return;
	}

	PlaybackState = EBGMPlaybackState::Transitioning;
	Current.Component->FadeOut(BGMTheme->FadeDuration, 0.f);
	Transition.Component->FadeIn(BGMTheme->FadeDuration, 1.f, Transition.StartTime);

	FTimerHandle FinishTimerHandle;
	const FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinishTransition);
	GetWorld()->GetTimerManager().SetTimer(FinishTimerHandle, FinishDelegate, BGMTheme->FadeDuration, false);
}

void UBGMManagerSubsystem::FinishTransition()
{
	StopSlot(Current);

	if (Transition.Component)
	{
		Transition.Component->OnAudioFinished.RemoveDynamic(this, &ThisClass::LoopTransition);
		Transition.Component->OnAudioFinished.AddDynamic(this, &ThisClass::LoopCurrent);
	}

	Current = Transition;
	Transition.Reset();
	PlaybackState = EBGMPlaybackState::Playing;

	if (PendingTrackType != NAME_None)
	{
		const EStageType RequestedStageType = PendingStageType;
		const FName RequestedTrackType = PendingTrackType;
		PendingStageType = EStageType::None;
		PendingTrackType = NAME_None;
		PlayBGM(RequestedStageType, RequestedTrackType);
	}
}

void UBGMManagerSubsystem::StopSlot(FBGMPlaybackSlot& Slot)
{
	if (Slot.Component)
	{
		Slot.Component->OnAudioFinished.RemoveDynamic(this, &ThisClass::LoopCurrent);
		Slot.Component->OnAudioFinished.RemoveDynamic(this, &ThisClass::LoopTransition);
		Slot.Component->Deactivate();
		Slot.Component->DestroyComponent();
	}

	Slot.Reset();
}

void UBGMManagerSubsystem::PlaySlot(FBGMPlaybackSlot& Slot, const EStageType StageType, const FName TrackType, USoundBase* Sound, const float StartTime, const bool bUseTransitionLoop)
{
	Slot.Component = UGameplayStatics::CreateSound2D(this, Sound, 1.f, 1.f, StartTime, nullptr, true, false);
	if (!Slot.Component)
	{
		return;
	}

	Slot.StageType = StageType;
	Slot.TrackType = TrackType;
	Slot.Duration = Sound->GetDuration();
	Slot.StartTime = FApp::GetCurrentTime() - StartTime;
	if (bUseTransitionLoop)
	{
		Slot.Component->OnAudioFinished.AddDynamic(this, &ThisClass::LoopTransition);
	}
	else
	{
		Slot.Component->OnAudioFinished.AddDynamic(this, &ThisClass::LoopCurrent);
		Slot.Component->Play(StartTime);
	}
}

float UBGMManagerSubsystem::GetCurrentTrackTime() const
{
	return FMath::Fmod(FApp::GetCurrentTime() - Current.StartTime, Current.Duration);
}

void UBGMManagerSubsystem::LoopCurrent()
{
	if (!Current.Component)
	{
		return;
	}

	Current.StartTime = FApp::GetCurrentTime();
	Current.Component->Play(0.f);
}

void UBGMManagerSubsystem::LoopTransition()
{
	if (!Transition.Component)
	{
		return;
	}

	Transition.StartTime = FApp::GetCurrentTime();
	Transition.Component->Play(0.f);
}
