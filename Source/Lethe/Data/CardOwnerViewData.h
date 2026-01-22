// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CardOwnerViewData.generated.h"

/**
 * 카드의 소유자(캐릭터)에 의해 결정되는 View와 관련된 에셋 묶음입니다.
 */
UCLASS()
class LETHE_API UCardOwnerViewData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CharacterTag;
	
	UPROPERTY(EditDefaultsOnly)
	FColor CardBacksideColor;
};
