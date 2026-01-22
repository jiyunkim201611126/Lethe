// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardOwnerViewData.h"

FPrimaryAssetId UCardOwnerViewData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(FPrimaryAssetType(TEXT("CardOwnerView")), GetFName());
}
