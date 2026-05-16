// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardDefinitionData.h"

FPrimaryAssetId UCardDefinitionData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CardDefinition")), GetFName());
}

int32 UCardDefinitionData::GetWeight(const int32 InLevel) const
{
	return CardWeight;
}
