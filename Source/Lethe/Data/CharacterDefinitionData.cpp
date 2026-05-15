// Copyright JETBLU, Inc. All Rights Reserved.

#include "CharacterDefinitionData.h"

FPrimaryAssetId UCharacterDefinitionData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CharacterDefinition")), GetFName());
}

int32 UCharacterDefinitionData::GetDeckCapacity(const int32 Level) const
{
	return BaseDeckCapacity + BonusDeckCapacityByLevel * FMath::Max(0, Level - 1);
}
