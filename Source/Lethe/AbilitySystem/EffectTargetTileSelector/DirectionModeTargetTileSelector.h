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

/** TargetGroup_Primary로 수집된 타일을 설명하는 구조체로, 추가 범위 선택 시 활용됩니다. */
struct FResolvedPrimaryTargetTile
{
	ATile* Tile = nullptr;
	int32 Direction = 0;
	int32 Distance = 0;
};

/**
 * 방향을 기반으로 선택하는 Selector입니다.
 * 방향 개수가 홀수일 땐 FCubeCoord에서 사용하는 Direction(0~5)의 의미를 그대로 사용합니다.
 * 짝수인 경우, ImpactPoint를 타일의 '경계'로 스냅해서 사용하기 때문에, 의미가 약간 달라져 'UpperDirection'이라는 이름으로 사용합니다.
 * UpperDirection이 D이면, 선택되는 방향은 (D - 1, D) Direction입니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FDirectionModeTargetTileSelector : public FEffectTargetTileSelector
{
	GENERATED_BODY()

public:
	FDirectionModeTargetTileSelector();
	
	virtual void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetTilesForAI(FEffectTargetTileSelectorContext& Context) const override;

protected:
	virtual void GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;
	virtual void GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const override;

private:
	/** 방향 개수가 짝수일 때, AI가 타일 중심이 아니라 두 방향 사이의 경계선쪽으로 조준하도록 그 위치 벡터를 반환합니다. */
	FVector MakeAIAimPointForEvenDirectionCount(const ATile* SourceTile, int32 UpperDirection) const;
	
	void HandleMeleeTargets(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const;
	void HandleStraightTargets(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const;
	void HandleParabolaTargets(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const;

	void HandleAdditionalTargets(FEffectTargetTileSelectorContext& Context, const TArray<FResolvedPrimaryTargetTile>& ResolvedPrimaryTargetTiles) const;
	
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
	void GetSelectedDirections(const ATile* SourceTile, const FTargetingIntent& TargetingIntent, TArray<int32>& OutDirections) const;
	
protected:
	/** TargetGroup_Primary를 채울 방식입니다.*/
	UPROPERTY(EditDefaultsOnly)
	ERangeType RangeType = ERangeType::Melee;

	/** 선택할 방향 개수입니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 DirectionCount = 1;

	/** 사거리입니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 RangeDistance = 1;

	/** 추가 범위입니다. */
	UPROPERTY(EditDefaultsOnly)
	TSet<EAdditionalRangeType> AdditionalRanges;
	
	/** 추가 범위에 대한 확장 카운트입니다. */
	UPROPERTY(EditDefaultsOnly)
	int32 ExtendCount = 0;
};
