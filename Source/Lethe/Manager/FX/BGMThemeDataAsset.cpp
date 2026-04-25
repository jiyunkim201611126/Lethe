// Copyright JETBLU, Inc. All Rights Reserved.

#include "BGMThemeDataAsset.h"

const FBGMTheme* UBGMThemeDataAsset::GetTheme(const EStageType StageType) const
{
	return BGMThemes.Find(StageType);
}
