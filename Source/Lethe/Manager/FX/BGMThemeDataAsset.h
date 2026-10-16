// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Lethe/Data/Stage/StageData.h"
#include "BGMThemeDataAsset.generated.h"

class USoundBase;
class USoundWave;

namespace BGM
{
	/** StemPlayerMetaSound가 받을 수 있는 최대 스템 수입니다. MetaSound 그래프의 슬롯 수와 일치해야 합니다. */
	inline constexpr int32 MaxStemCount = 5;
}

/**
 * 하나의 곡을 악기군별로 분리한 오디오 파일입니다.
 * 같은 테마의 모든 스템은 길이와 시작점이 같아야 동시에 재생했을 때 박자가 맞습니다.
 */
USTRUCT(BlueprintType)
struct FBGMStem
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName StemName;

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundWave> Wave;
};

/**
 * 스템별 볼륨 조합입니다. 전투/비전투 등 상황에 따라 프리셋을 바꿔 곡의 분위기를 조절합니다.
 */
USTRUCT(BlueprintType)
struct FBGMMixPreset
{
	GENERATED_BODY()

	/** Stems와 같은 순서의 스템별 볼륨(0 = 뮤트)입니다. 에디터에서 Stems 수에 맞춰 자동으로 크기가 조정됩니다. */
	UPROPERTY(EditDefaultsOnly, EditFixedSize, meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	TArray<float> StemVolumes;

	/** 항목이 없는 스템은 원래 볼륨(1)으로 재생합니다. */
	float GetStemVolume(const int32 StemIndex) const
	{
		return StemVolumes.IsValidIndex(StemIndex) ? StemVolumes[StemIndex] : 1.f;
	}
};

/**
 * 스테이지 하나의 BGM입니다. 테마 하나가 곧 곡 하나이며, 상황에 따른 변화는 MixPresets로 표현합니다.
 */
USTRUCT(BlueprintType)
struct FBGMTheme
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TArray<FBGMStem> Stems;

	/** 프리셋이 바뀔 때의 스템 볼륨 페이드 시간이자, 다른 테마에서 이 테마로 넘어올 때의 크로스페이드 시간(초)입니다. */
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", Units = "Seconds"))
	float FadeDuration = 2.f;

	/** 요청된 프리셋이 이 테마에 없을 때 사용할 프리셋입니다. */
	UPROPERTY(EditDefaultsOnly)
	FName DefaultMixPreset;

	/** 프리셋 이름(Idle, Battle 등) -> 스템별 볼륨입니다. */
	UPROPERTY(EditDefaultsOnly)
	TMap<FName, FBGMMixPreset> MixPresets;

	/** PresetName에 해당하는 프리셋을 찾고, bFallbackToDefault라면 없을 때 DefaultMixPreset을 반환합니다. */
	const FBGMMixPreset* FindMixPreset(const FName PresetName, const bool bFallbackToDefault) const;
};

UCLASS()
class LETHE_API UBGMThemeDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
#if WITH_EDITOR
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnThemeEdited, const UBGMThemeDataAsset* /*EditedAsset*/);

	/**
	 * 에디터에서 값이 바뀔 때마다 호출됩니다. PIE 중인 BGMManagerSubsystem이 구독해 재생 중인 곡에 즉시 반영합니다.
	 * 런타임 모듈이 에디터 모듈에 의존하지 않도록 에셋 쪽에서 알리는 방식을 사용합니다.
	 */
	static FOnThemeEdited OnThemeEdited;
#endif

	const FBGMTheme* GetTheme(const EStageType StageType) const;
	USoundBase* GetStemPlayerMetaSound() const { return StemPlayerMetaSound; }

	//~ Begin UObject Interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	//~ End of UObject Interface

protected:
	/**
	 * 모든 테마의 스템 재생에 사용하는 공용 MetaSound입니다.
	 * 입력: Stem{N}_Wave(WaveAsset), Stem{N}_Volume(Float), FadeTime(Time) — N은 0 ~ MaxStemCount-1
	 */
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<USoundBase> StemPlayerMetaSound;

	UPROPERTY(EditDefaultsOnly)
	TMap<EStageType, FBGMTheme> BGMThemes;
};
