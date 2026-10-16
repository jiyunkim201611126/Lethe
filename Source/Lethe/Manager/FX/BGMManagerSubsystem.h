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

	void Reset()
	{
		Component = nullptr;
		StageType = EStageType::None;
	}
};

UCLASS(Config = Game)
class LETHE_API UBGMManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (ToolTip = "StageType 테마의 BGM으로 크로스페이드합니다. 이미 전환 중이라면 해당 전환이 끝난 뒤 시작합니다."))
	void RequestPlayBGM(const EStageType StageType);

	UFUNCTION(BlueprintCallable, meta = (ToolTip = "재생 중인 BGM의 스템별 볼륨을 PresetName 프리셋으로 페이드 전환합니다. 이후 새로 재생되는 테마에도 같은 프리셋이 적용됩니다."))
	void RequestBGMMixPreset(const FName PresetName);

private:
	const FBGMTheme* LoadTheme(const EStageType StageType);
	void PrepareSlot(FBGMPlaybackSlot& Slot, const EStageType StageType, const FBGMTheme& Theme) const;
	void StopSlot(FBGMPlaybackSlot& Slot) const;

	void StartTransition(const EStageType StageType, const FBGMTheme& Theme);
	void FinishTransition();

	/** 재생 중인 슬롯에 프리셋을 적용합니다. 이미 재생 중인 슬롯이라 페이드가 적용됩니다. */
	void ApplyMixPresetToSlot(const FBGMPlaybackSlot& Slot, const FName PresetName) const;
	static void SetStemVolumes(UAudioComponent* Component, const FBGMTheme& Theme, const FBGMMixPreset* Preset, const float FadeTime);

private:
	UPROPERTY(Config)
	TSoftObjectPtr<UBGMThemeDataAsset> BGMDataAssetPath;

	UPROPERTY()
	TObjectPtr<UBGMThemeDataAsset> BGMDataAsset;

	UPROPERTY()
	FBGMPlaybackSlot Current;

	UPROPERTY()
	FBGMPlaybackSlot Transition;

	/** 전환 중에 들어온 요청입니다. 전환이 끝나면 재생합니다. */
	EStageType PendingStageType = EStageType::None;

	/** 마지막으로 요청된 믹스 프리셋입니다. 테마가 바뀌어도 유지되어, 새 테마도 현재 상황(전투/비전투)에 맞게 시작합니다. */
	FName RequestedMixPreset;

	FTimerHandle TransitionFinishTimerHandle;
	EBGMPlaybackState PlaybackState = EBGMPlaybackState::Stopped;
};
