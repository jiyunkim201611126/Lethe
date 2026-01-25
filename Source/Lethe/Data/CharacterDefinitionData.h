// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CharacterDefinitionData.generated.h"

class APlayerCharacterBase;
/**
 * 캐릭터의 정의와 관련된 데이터 묶음입니다.
 */
UCLASS()
class LETHE_API UCharacterDefinitionData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

public:
	// Id는 출시 이후 절대 변경되어선 안 됩니다!!
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	uint64 CharacterId;
	
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	FGameplayTag CharacterTag;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<APlayerCharacterBase> CharacterClass;
	
	UPROPERTY(EditDefaultsOnly)
	FColor CardBacksideColor;
};
