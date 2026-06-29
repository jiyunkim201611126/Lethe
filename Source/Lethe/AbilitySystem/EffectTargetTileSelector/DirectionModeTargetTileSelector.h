// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectTargetTileSelector.h"
#include "DirectionModeTargetTileSelector.generated.h"

UENUM(BlueprintType)
enum class ERangeType : uint8
{
	Melee,
	StraightRanged,
	ParabolaRanged,
};

UENUM(BlueprintType)
enum class EAdditionalRangeType : uint8
{
	Penetration,
};

UCLASS()
class LETHE_API UDirectionModeTargetTileSelector : public UEffectTargetTileSelector
{
	GENERATED_BODY()

public:
	virtual void GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const override;
	virtual void GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const override;
	virtual void GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const override;
	
protected:
	UPROPERTY(EditDefaultsOnly)
	ERangeType RangeType = ERangeType::Melee;
	
	UPROPERTY(EditDefaultsOnly)
	int32 DirectionCount = 1;
	
	UPROPERTY(EditDefaultsOnly)
	int32 RangeEnforceValue = 1;

	/** Value는 EnforceValue입니다. */
	UPROPERTY(EditDefaultsOnly)
	TMap<EAdditionalRangeType, int32> AdditionalRangeTypes;
};
