// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectTargetTileSelector.h"
#include "TileModeTargetTileSelector.generated.h"

UENUM(BlueprintType)
enum class ETargetTeamRelation : uint8
{
	AllSides,
	SameTeam,
	OpposingTeam,
};

USTRUCT(BlueprintType)
struct LETHE_API FTileModeTargetTileSelector : public FEffectTargetTileSelector
{
	GENERATED_BODY()

	virtual void GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles) const override;
	virtual void GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const override;

protected:
	virtual void GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const override;
	virtual void GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const override;

protected:
	/** 타일 선택 가능 범위입니다. */
	UPROPERTY(EditDefaultsOnly)
	FBFSRange SelectRange;

	/** 타일 선택 후, 해당 타일을 기준으로 어떤 타일을 선택할지 정하는 범위입니다. */
	UPROPERTY(EditDefaultsOnly)
	FBFSRange TargetTileRange;
	
	UPROPERTY(EditDefaultsOnly)
	ETargetTeamRelation TargetTeamRelation = ETargetTeamRelation::AllSides;
};
