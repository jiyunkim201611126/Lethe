// Copyright JETBLU, Inc. All Rights Reserved.

#include "CharacterDefinitionData.h"

FPrimaryAssetId UCharacterDefinitionData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CharacterDefinition")), GetFName());
}
