// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardDefinitionData.h"

FPrimaryAssetId UCardDefinitionData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CardDefinition")), CardTag.GetTagName());
}
