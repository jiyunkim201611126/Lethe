// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMManagerSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"

void UBGMManagerSubsystem::PlayBGM(const EStageType StageType, const FName TrackType)
{
	if (!BGMDataAsset)
	{
		BGMDataAsset = BGMDataAssetPath.LoadSynchronous();
	}
	
	if (!BGMDataAsset)
	{
		LETHE_LOG(LogBGMManager, Error, "BGMDataAssetPath가 설정되지 않았습니다.");
		return;
	}
	
	const FBGMTheme* CurrentBGMTheme = BGMDataAsset->GetTheme(StageType);
	if (!CurrentBGMTheme)
	{
		LETHE_LOG(LogBGMManager, Error, "%s에 해당하는 Stage가 없습니다.", *LogHelper::EnumToString(StageType));
		return;
	}

	USoundBase* BGMSoundAsset = CurrentBGMTheme->Tracks.FindRef(TrackType);
	if (!BGMSoundAsset)
	{
		return;
	}

	if (!CurrentComponent)
	{
		// 처음 BGM을 재생하는 경우 들어오는 분기입니다.
		CurrentComponent = UGameplayStatics::SpawnSound2D(this, BGMSoundAsset, 1.f, 1.f, 0.f, nullptr, true, false);
		if (CurrentComponent)
		{
			AudioStartTime = FApp::GetCurrentTime();
			CurrentAudioTrackLength = CurrentBGMTheme->BGMTrackLength;
		}
		return;
	}

	// 현재 재생 중인 BGM과 동일한 BGM을 재생하려 하는 경우 들어가는 분기입니다.
	if (CurrentComponent->GetSound() == BGMSoundAsset)
	{
		// 아직 Transition이 진행되지 않았다면 대기 상태인 BGM들을 모두 폐기합니다.
		if (!bIsTransitioning)
		{
			GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
			TransitionBGMStageType = EStageType::None;
			if (TransitionComponent)
			{
				TransitionComponent->Deactivate();
				TransitionComponent->DestroyComponent();
				TransitionComponent = nullptr;
			}
			PendingBGMStageType = EStageType::None;
			if (PendingComponent)
			{
				PendingComponent->Deactivate();
				PendingComponent->DestroyComponent();
				PendingComponent = nullptr;
			}
			return;
		}
	}

	// 중복 요청인 경우 무시합니다.
	if (TransitionComponent && TransitionComponent->GetSound() == BGMSoundAsset)
	{
		return;
	}

	// 현재 재생 중인 BGM의 재생 지점 시간을 계산합니다.
	const double CurrentTime = FApp::GetCurrentTime();
	const float CurrentAudioTrackTime = FMath::Fmod(CurrentTime - AudioStartTime, CurrentAudioTrackLength);
	
	// 현재 Transition 중이 아닌 경우 들어가는 분기입니다.
	if (!bIsTransitioning)
	{
		// 이미 걸어뒀던 타이머는 해제합니다.
		GetWorld()->GetTimerManager().ClearTimer(TransitionTimerHandle);
		
		// 현재 재생 중인 BGM과 동일한 지점에서 재생을 시작합니다.
		TransitionBGMStageType = StageType;
		if (TransitionComponent)
		{
			TransitionComponent->Deactivate();
			TransitionComponent->DestroyComponent();
		}
		TransitionComponent = UGameplayStatics::CreateSound2D(this, BGMSoundAsset, 1.f, 1.f, CurrentAudioTrackTime, nullptr, true, false);
		if (TransitionComponent)
		{
			TransitionComponent->FadeIn(0.f, 0.f, CurrentAudioTrackTime);

			// 가장 가까운 TransitionPoint에서 Transition이 시작될 수 있도록 타이머를 걸어줍니다.
			const float TransitionStartDelay = GetTransitionStartDelay(CurrentAudioTrackTime);
			SetTransitionTimer(TransitionStartDelay);
		}
		return;
	}

	// 이미 Transition 중이라면 대기시킵니다.
	PendingBGMStageType = StageType;
	if (PendingComponent)
	{
		PendingComponent->Deactivate();
		PendingComponent->DestroyComponent();
	}
	PendingComponent = UGameplayStatics::CreateSound2D(this, BGMSoundAsset, 1.f, 1.f, CurrentAudioTrackTime, nullptr, true, false);
	if (PendingComponent)
	{
		PendingComponent->FadeIn(0.f, 0.f, CurrentAudioTrackTime);
	}
}

float UBGMManagerSubsystem::GetTransitionStartDelay(const float CurrentAudioTrackTime) const
{
	if (TransitionComponent)
	{
		const FBGMTheme* BGMTheme = BGMDataAsset->GetTheme(TransitionBGMStageType);
		if (!BGMTheme)
		{
			return 0.f;
		}

		// 현재 재생 시간을 기준으로 가장 가까운 TransitionPoint를 반환합니다.
		for (const float TransitionPoint : BGMTheme->TransitionPoints)
		{
			if (CurrentAudioTrackTime < TransitionPoint)
			{
				return TransitionPoint - CurrentAudioTrackTime;
			}
		}

		// 이미 TransitionPoint를 모두 지나쳤다면, 남은 시간과 가장 앞에 있는 TransitionPoint를 더해서 반환합니다.
		if (!BGMTheme->TransitionPoints.IsEmpty())
		{
			const float RemainTrackTime = CurrentAudioTrackLength - CurrentAudioTrackTime; 
			const float FirstTransitionPoint = BGMTheme->TransitionPoints[0];
			return RemainTrackTime + FirstTransitionPoint;
		}
	}

	return 0.f;
}

void UBGMManagerSubsystem::SetTransitionTimer(const float TransitionStartDelay)
{
	const FTimerDelegate TransitionTimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::StartTransition);
	GetWorld()->GetTimerManager().SetTimer(TransitionTimerHandle, TransitionTimerDelegate, TransitionStartDelay, false);
}

void UBGMManagerSubsystem::StartTransition()
{
	const FBGMTheme* BGMTheme = BGMDataAsset->GetTheme(TransitionBGMStageType);
	if (!BGMTheme)
	{
		return;
	}
	
	if (BGMTheme && CurrentComponent && TransitionComponent)
	{
		const USoundBase* TransitionSound = TransitionComponent->GetSound();
		if (!TransitionSound)
		{
			return;
		}

		const float NextTrackLength = BGMTheme->BGMTrackLength;

		// BGM이 변경되었으므로 기록된 시간들을 변경된 BGM에 맞춰 갱신합니다.
		const double CurrentTime = FApp::GetCurrentTime();
		const float CurrentAudioTrackTime = FMath::Fmod(CurrentTime - AudioStartTime, CurrentAudioTrackLength);
		AudioStartTime = CurrentTime - CurrentAudioTrackTime;
		CurrentAudioTrackLength = NextTrackLength;
		
		bIsTransitioning = true;
		const float FadeDuration = BGMTheme->FadeDuration;
		CurrentComponent->FadeOut(FadeDuration, 0.f);
		TransitionComponent->AdjustVolume(FadeDuration, 1.f);

		FTimerHandle OnTransitionEndedTimerHandle;
		const FTimerDelegate OnTransitionEndedTimerDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::OnTransitionEnded);
		GetWorld()->GetTimerManager().SetTimer(OnTransitionEndedTimerHandle, OnTransitionEndedTimerDelegate, FadeDuration, false);
	}
}

void UBGMManagerSubsystem::OnTransitionEnded()
{
	bIsTransitioning = false;
	TransitionBGMStageType = EStageType::None;
	if (CurrentComponent)
	{
		CurrentComponent->Deactivate();
		CurrentComponent->DestroyComponent();
	}
	CurrentComponent = TransitionComponent;
	TransitionComponent = nullptr;

	if (PendingComponent)
	{
		TransitionBGMStageType = PendingBGMStageType;
		TransitionComponent = PendingComponent;
		PendingComponent = nullptr;
		PendingBGMStageType = EStageType::None;
		
		const double CurrentTime = FApp::GetCurrentTime();
		const float CurrentAudioTrackTime = FMath::Fmod(CurrentTime - AudioStartTime, CurrentAudioTrackLength);
		const float TransitionStartDelay = GetTransitionStartDelay(CurrentAudioTrackTime);
		SetTransitionTimer(TransitionStartDelay);
	}
}
