// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Lethe/Manager/World/LevelManagerSubsystem.h"
#include "LevelData.generated.h"

enum class ELevelType : uint8;

USTRUCT(BlueprintType)
struct FLevelInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	ELevelType LevelType = ELevelType::None;

	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UWorld> LevelAsset;
};

/**
 * ELevelType과 Level 에셋을 매핑하는 데에 사용되는 DataAsset입니다.
 */
UCLASS()
class LETHE_API ULevelData : public UDataAsset
{
	GENERATED_BODY()

public:
	TSoftObjectPtr<UWorld> GetLevelAssetByType(const ELevelType InLevelType) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TArray<FLevelInfo> LevelList;
};
