// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMManagerSubsystem.h"

#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"

namespace BGMStemParam
{
	// StemPlayerMetaSound 그래프의 입력 이름과 일치해야 합니다.
	static FName GetWaveName(const int32 StemIndex)
	{
		return FName(*FString::Printf(TEXT("Stem%d_Wave"), StemIndex));
	}

	static FName GetVolumeName(const int32 StemIndex)
	{
		return FName(*FString::Printf(TEXT("Stem%d_Volume"), StemIndex));
	}

	static FName GetFadeTimeName()
	{
		static const FName Name(TEXT("FadeTime"));
		return Name;
	}
}

void UBGMManagerSubsystem::RequestPlayBGM(const EStageType StageType)
{
	const FBGMTheme* Theme = LoadTheme(StageType);
	if (!Theme)
	{
		return;
	}

	// 아무것도 재생하고 있지 않은 상태라면 Current 슬롯에 넣어 바로 재생합니다.
	if (PlaybackState == EBGMPlaybackState::Stopped)
	{
		PrepareSlot(Current, StageType, *Theme);
		if (Current.Component)
		{
			Current.Component->Play(0.f);
			PlaybackState = EBGMPlaybackState::Playing;
		}
		return;
	}

	// 전환 중이라면 전환이 끝난 뒤 처리합니다. 전환 대상과 같은 BGM이라면 대기 중인 요청만 지웁니다.
	if (PlaybackState == EBGMPlaybackState::Transitioning)
	{
		PendingStageType = Transition.StageType == StageType ? EStageType::None : StageType;
		return;
	}

	// 이미 같은 BGM을 재생 중이라면 무시합니다.
	if (Current.StageType == StageType)
	{
		return;
	}

	StartTransition(StageType, *Theme);
}

void UBGMManagerSubsystem::RequestBGMMixPreset(const FName PresetName)
{
	if (PresetName == RequestedMixPreset)
	{
		return;
	}

	RequestedMixPreset = PresetName;

	// 전환 중이라면 사라질 곡과 새로 들어오는 곡 모두에 적용합니다.
	ApplyMixPresetToSlot(Current, PresetName);
	ApplyMixPresetToSlot(Transition, PresetName);
}

const FBGMTheme* UBGMManagerSubsystem::LoadTheme(const EStageType StageType)
{
	if (!BGMDataAsset)
	{
		BGMDataAsset = BGMDataAssetPath.LoadSynchronous();
	}

	if (!BGMDataAsset)
	{
		LETHE_LOG(LogBGMManager, Error, "BGMDataAssetPath가 설정되지 않았습니다.");
		return nullptr;
	}

	if (!BGMDataAsset->GetStemPlayerMetaSound())
	{
		LETHE_LOG(LogBGMManager, Error, "StemPlayerMetaSound가 설정되지 않았습니다.");
		return nullptr;
	}

	const FBGMTheme* Theme = BGMDataAsset->GetTheme(StageType);
	if (!Theme)
	{
		LETHE_LOG(LogBGMManager, Error, "%s에 해당하는 Theme가 없습니다.", *LogHelper::EnumToString(StageType));
		return nullptr;
	}

	if (Theme->Stems.IsEmpty() || Theme->Stems.Num() > BGM::MaxStemCount)
	{
		LETHE_LOG(LogBGMManager, Error, "%s Theme의 스템 수(%d)가 유효하지 않습니다. (1 ~ %d)", *LogHelper::EnumToString(StageType), Theme->Stems.Num(), BGM::MaxStemCount);
		return nullptr;
	}

	for (const FBGMStem& Stem : Theme->Stems)
	{
		if (!Stem.Wave)
		{
			LETHE_LOG(LogBGMManager, Error, "%s Theme의 %s 스템에 Wave가 없습니다.", *LogHelper::EnumToString(StageType), *Stem.StemName.ToString());
			return nullptr;
		}
	}

	return Theme;
}

void UBGMManagerSubsystem::PrepareSlot(FBGMPlaybackSlot& Slot, const EStageType StageType, const FBGMTheme& Theme) const
{
	// 사운드를 생성해두기만 하고, 실제 재생은 호출한 쪽에서 수행합니다.
	Slot.Component = UGameplayStatics::CreateSound2D(this, BGMDataAsset->GetStemPlayerMetaSound(), 1.f, 1.f, 0.f, nullptr, true, false);
	if (!Slot.Component)
	{
		return;
	}

	// 파라미터는 재생 전에 설정해두면 재생 시작과 동시에 적용됩니다.
	for (int32 StemIndex = 0; StemIndex < Theme.Stems.Num(); ++StemIndex)
	{
		Slot.Component->SetObjectParameter(BGMStemParam::GetWaveName(StemIndex), Theme.Stems[StemIndex].Wave);
	}

	// 시작 볼륨은 보간 없이 바로 프리셋 값으로 맞춥니다. 곡 전체의 페이드인은 AudioComponent의 FadeIn이 담당합니다.
	SetStemVolumes(Slot.Component, Theme, Theme.FindMixPreset(RequestedMixPreset, true), 0.f);

	Slot.StageType = StageType;
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

void UBGMManagerSubsystem::StartTransition(const EStageType StageType, const FBGMTheme& Theme)
{
	PrepareSlot(Transition, StageType, Theme);
	if (!Transition.Component)
	{
		StopSlot(Transition);
		return;
	}

	// 곡 전환을 시작합니다.
	PlaybackState = EBGMPlaybackState::Transitioning;
	if (Current.Component)
	{
		Current.Component->FadeOut(Theme.FadeDuration, 0.f);
	}
	Transition.Component->FadeIn(Theme.FadeDuration, 1.f, 0.f);

	// 타이머는 0초로 설정하면 실행되지 않으므로, 페이드가 없으면 바로 마무리합니다.
	if (Theme.FadeDuration <= 0.f)
	{
		FinishTransition();
		return;
	}

	const FTimerDelegate FinishDelegate = FTimerDelegate::CreateUObject(this, &ThisClass::FinishTransition);
	GetGameInstance()->GetTimerManager().SetTimer(TransitionFinishTimerHandle, FinishDelegate, Theme.FadeDuration, false);
}

void UBGMManagerSubsystem::FinishTransition()
{
	StopSlot(Current);

	Current = Transition;
	Transition.Reset();

	PlaybackState = EBGMPlaybackState::Playing;

	if (PendingStageType != EStageType::None)
	{
		const EStageType RequestedStageType = PendingStageType;
		PendingStageType = EStageType::None;
		RequestPlayBGM(RequestedStageType);
	}
}

void UBGMManagerSubsystem::ApplyMixPresetToSlot(const FBGMPlaybackSlot& Slot, const FName PresetName) const
{
	if (!Slot.Component || !BGMDataAsset)
	{
		return;
	}

	const FBGMTheme* Theme = BGMDataAsset->GetTheme(Slot.StageType);
	if (!Theme)
	{
		return;
	}

	// 재생 중인 곡에 없는 프리셋이면 기본 프리셋으로 되돌리지 않고 현재 볼륨을 유지합니다.
	const FBGMMixPreset* Preset = Theme->FindMixPreset(PresetName, false);
	if (!Preset)
	{
		LETHE_LOG(LogBGMManager, Warning, "%s Theme에 %s 프리셋이 없습니다.", *LogHelper::EnumToString(Slot.StageType), *PresetName.ToString());
		return;
	}

	SetStemVolumes(Slot.Component, *Theme, Preset, Theme->FadeDuration);
}

void UBGMManagerSubsystem::SetStemVolumes(UAudioComponent* Component, const FBGMTheme& Theme, const FBGMMixPreset* Preset, const float FadeTime)
{
	// FadeTime을 볼륨보다 먼저 보내야 MetaSound가 새 볼륨으로 보간할 때 이 시간을 사용합니다.
	Component->SetFloatParameter(BGMStemParam::GetFadeTimeName(), FadeTime);
	for (int32 StemIndex = 0; StemIndex < Theme.Stems.Num(); ++StemIndex)
	{
		const float Volume = Preset ? Preset->GetStemVolume(StemIndex) : 1.f;
		Component->SetFloatParameter(BGMStemParam::GetVolumeName(StemIndex), Volume);
	}
}
