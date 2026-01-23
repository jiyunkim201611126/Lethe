// Copyright JETBLU, Inc. All Rights Reserved.

#include "LevelData.h"

TSoftObjectPtr<UWorld> ULevelData::GetLevelAssetByType(const ELevelType InLevelType) const
{
	for (const FLevelInfo& LevelInfo : LevelList)
	{
		if (LevelInfo.LevelType == InLevelType)
		{
			return LevelInfo.LevelAsset;
		}
	}
	return nullptr;
}
