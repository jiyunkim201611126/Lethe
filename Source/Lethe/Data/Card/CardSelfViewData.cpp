// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardSelfViewData.h"

FPrimaryAssetId UCardSelfViewData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CardSelfView")), GetFName());
}
