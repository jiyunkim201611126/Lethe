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

/**
 * FBFSRange 기반으로 타일을 선택하는 Selector입니다.
 * TargetTiles는 중심 타일부터 출발해 좌상단, 반시계방향을 반복해 인덱스를 채워나갑니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FTileModeTargetTileSelector : public FEffectTargetTileSelector
{
	GENERATED_BODY()

	virtual void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const override;

protected:
	virtual void GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;

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
