// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMThemeDataAsset.h"

const FBGMMixPreset* FBGMTheme::FindMixPreset(const FName PresetName, const bool bFallbackToDefault) const
{
	if (const FBGMMixPreset* Preset = MixPresets.Find(PresetName))
	{
		return Preset;
	}

	return bFallbackToDefault ? MixPresets.Find(DefaultMixPreset) : nullptr;
}

const FBGMTheme* UBGMThemeDataAsset::GetTheme(const EStageType StageType) const
{
	return BGMThemes.Find(StageType);
}

#if WITH_EDITOR
UBGMThemeDataAsset::FOnThemeEdited UBGMThemeDataAsset::OnThemeEdited;

void UBGMThemeDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 프리셋의 볼륨 배열을 스템 수에 맞춥니다. 새로 생긴 칸은 원래 볼륨(1)으로 채웁니다.
	for (TPair<EStageType, FBGMTheme>& ThemePair : BGMThemes)
	{
		const int32 StemCount = ThemePair.Value.Stems.Num();
		for (TPair<FName, FBGMMixPreset>& PresetPair : ThemePair.Value.MixPresets)
		{
			TArray<float>& StemVolumes = PresetPair.Value.StemVolumes;
			const int32 OldCount = StemVolumes.Num();
			StemVolumes.SetNum(StemCount);
			for (int32 Index = OldCount; Index < StemCount; ++Index)
			{
				StemVolumes[Index] = 1.f;
			}
		}
	}

	// 배열 크기를 맞춘 뒤에 알려야 구독자가 정리된 값을 읽습니다.
	OnThemeEdited.Broadcast(this);
}
#endif
