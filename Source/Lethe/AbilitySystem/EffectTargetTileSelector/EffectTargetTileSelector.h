// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetSelectionData.h"
#include "EffectTargetTileSelector.generated.h"

class APlayerController;
class ATile;
class UTileManagerSubsystem;
struct FTargetSelectionResult;

UENUM(BlueprintType)
enum class ETargetTeamRelation : uint8
{
	AllSides,
	SameTeam,
	OpposingTeam,
};

/**
 * Ability 적용 범위에 포함되는 타일과, 해당 타일에서 Effect를 적용할 수 있는 Actor를 수집합니다.
 * ActorOnTile은 타일이 비어있거나 대상 조건을 만족하지 않으면 유효하지 않습니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FEffectTargetTileSelector
{
	GENERATED_BODY()

public:
	virtual ~FEffectTargetTileSelector() = default;

	/** 선택 가능 타일과 적용될 대상이 될 수 있는 후보 타일을 OutResult에 반환합니다. */
	virtual void GetCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const;

	/** 시전 결과에 포함되는 타일과 유효한 대상 Actor를 OutResult에 반환합니다. */
	virtual void GetTargetTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const;

	/**
	 * 기본 구현은 GetTargetTiles를 호출하며, 필요한 경우 하위 구조체가 오버라이드해 구현합니다.
	 * Context 내 TargetingIntent를 보정해서 사용하는 경우도 있기 때문에 호출 후 TargetingIntent를 재사용하려는 경우 주의가 필요합니다.
	 */
	virtual void GetTargetTilesForAI(FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const;

protected:
	/** 시전 가능 범위에 해당하는 타일들을 OutResult에 반환합니다. */
	virtual void GetSelectCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const;

	/** 시전 시 적용 범위에 해당하는 타일들을 모두 OutResult에 반환합니다. */
	virtual void GetTargetCandidateTiles(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const;

	/** 
	 * OutTargetCandidates를 기반으로 Actor를 수집하고, 중복 대상을 제거하며 TargetTeamRelation 규칙에 일치하는 대상만 남깁니다.
	 * 타일 위에 Actor가 없으면 제거하며, 동일 타일이 수집된 경우는 Primary를 우선으로 남기며 제거합니다.
	 */
	void ResolveTargets(const FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	ETargetTeamRelation TargetTeamRelation = ETargetTeamRelation::OpposingTeam;
};
