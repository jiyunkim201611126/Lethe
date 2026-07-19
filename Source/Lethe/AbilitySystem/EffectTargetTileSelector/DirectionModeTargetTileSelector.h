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
	HalfMoon,
	Spread,
};

/** TargetTileGroup_Primary로 수집된 타일을 설명하는 구조체로, 추가 범위 선택 시 활용됩니다. */
struct FResolvedPrimaryTargetTile
{
	ATile* Tile = nullptr;
	int32 Direction = 0;
	int32 Distance = 0;
};

/**
 * 방향을 기반으로 선택하는 Selector입니다.
 * 반시계 방향으로 회전하며 인덱스를 채워나갑니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FDirectionModeTargetTileSelector : public FEffectTargetTileSelector
{
	GENERATED_BODY()

	virtual void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const override;

protected:
	virtual void GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;

private:
	void HandleMeleeRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const;
	void HandleParabolaRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const;
	void HandleStraightRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const;

	void HandleAdditionalRanges(FEffectTargetTileSelectorContext& Context, const TArray<FResolvedPrimaryTargetTile>& ResolvedPrimaryTargetTiles) const;
	
	/** FCubeCoord 기준 Direction을 방향 Vector로 변환해 반환합니다. */
	FVector2D GetHexDirectionVector(int32 Direction) const;
	
	/** FCubeCoord 기준 Direction 0 ~ 5 중, 가장 가까운 방향으로 스냅해 반환합니다. */
	int32 FindClosestHexDirection(const FVector2D& DesiredDirection) const;
	
	/**
	 * FCubeCoord 기준 Direction 0 ~ 5 그 사이 경계 방향 Vector 중 가장 가까운 방향으로 스냅해 반환합니다.
	 * 0: 위쪽 / 1: 좌상단과 좌측 사이 ... / 5: 우측과 우상단 사이
	 */
	int32 FindClosestHexDirectionBoundary(const FVector2D& DesiredDirection) const;

	/** 원하는 개수만큼의 방향을 Out 인자로 반환합니다. */
	void GetSelectedDirections(const ATile* CurrentTile, const APlayerController* PlayerController, TArray<int32>& OutDirections) const;
	
protected:
	/** TargetTileGroup_Primary를 채울 방식입니다.*/
	UPROPERTY(EditDefaultsOnly)
	ERangeType RangeType = ERangeType::Melee;

	/** 선택할 방향 개수입니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 DirectionCount = 1;

	/** 사거리입니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 RangeDistance = 1;

	/** Key는 TargetTileGroup_Primary를 제외한 다른 추가 범위를 의미하며, Value는 EnforceValue입니다. */
	UPROPERTY(EditDefaultsOnly)
	TMap<EAdditionalRangeType, int32> AdditionalRanges;
};
