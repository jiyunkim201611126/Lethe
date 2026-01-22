// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
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
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	FGameplayTag CardTag;
	
	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditDefaultsOnly)
	FText CardNameText;
	
	// 아래 Text는 런타임 중 Ability를 참조해 동적으로 채워집니다.
	FText CardDescriptionText;
};
