// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CardSelfViewData.generated.h"

/**
 * 카드 자신(Ability)에 의해 결정되는 View와 관련된 에셋 묶음입니다.
 */
UCLASS()
class LETHE_API UCardSelfViewData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	// Id는 출시 이후 절대 변경되어선 안 됩니다!!
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	uint64 CardId;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditDefaultsOnly)
	FText CardNameText;
};
