// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CardDefinitionData.generated.h"

class ULetheGameplayAbility;
/**
 * 카드 태그와 Ability CDO를 매핑하는 데이터 에셋입니다.
 */
UCLASS()
class LETHE_API UCardDefinitionData : public UPrimaryDataAsset
{
	GENERATED_BODY()
	
public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;
	
public:
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	FGameplayTag CardTag;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag CardTypeTag;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ULetheGameplayAbility> AbilityClass;
};
