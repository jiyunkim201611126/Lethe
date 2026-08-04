// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectTargetTileSelector.h"
#include "Lethe/Data/Stage/TileData.h"
#include "TileModeTargetTileSelector.generated.h"

/**
 * FBFSRange 기반으로 타일을 선택하는 Selector입니다.
 * TargetTiles는 중심 타일부터 출발해 선정하지만, 인덱스 순서가 보장되지 않습니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FTileModeTargetTileSelector : public FEffectTargetTileSelector
{
	GENERATED_BODY()

public:
	virtual void GetCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const override;
	virtual void GetTargetTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const override;

protected:
	virtual void GetSelectCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const override;
	virtual void GetTargetCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const override;

protected:
	/** 타일 선택 가능 범위입니다. */
	UPROPERTY(EditDefaultsOnly)
	FBFSRange SelectRange;

	/** 타일 선택 후, 해당 타일을 기준으로 어떤 타일을 선택할지 정하는 범위입니다. */
	UPROPERTY(EditDefaultsOnly)
	FBFSRange TargetTileRange;
};
