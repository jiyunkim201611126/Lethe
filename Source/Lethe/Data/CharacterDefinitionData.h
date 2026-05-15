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

	int32 GetDeckCapacity(const int32 Level) const;

public:
	/** ※!! Id는 출시 이후 절대 변경되어선 안 됩니다  !!※ */
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	uint64 CharacterId;
	
	UPROPERTY(EditDefaultsOnly, AssetRegistrySearchable)
	FGameplayTag CharacterTag;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<APlayerCharacterBase> CharacterClass;

	/** 캐릭터의 PersonalColor이며, 카드의 뒷면에도 사용하는 색깔입니다. */
	UPROPERTY(EditDefaultsOnly)
	FColor PersonalColor;

	/** 기본 카드 용량입니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 BaseDeckCapacity = 20;

	/** 레벨업에 따른 추가 카드 용량입니다. Curve로 변경될 수 있습니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 BonusDeckCapacityByLevel = 2;
};
