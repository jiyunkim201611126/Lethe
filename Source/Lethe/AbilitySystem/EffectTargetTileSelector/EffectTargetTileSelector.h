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
 * CandidateTiles나 TargetTiles가 필요한 경우 CardAbility의 함수를 호출하며 사용하는 구조체입니다.
 * 외부에서는 AvatarActor와 TargetingIntent만 채워서 호출하면 나머지는 CardAbility와 TargetTileSelector가 채워줍니다.
 */
struct FEffectTargetTileSelectorContext
{
	const AActor* AvatarActor = nullptr;
	FTargetingIntent TargetingIntent;

	const UTileManagerSubsystem* TileManagerSubsystem = nullptr;
	const ATile* SourceTile = nullptr;

	TArray<ATile*> OutSelectCandidateTiles;
	TArray<FTargetSelectionResult> OutTargetResults;

	bool IsValid() const
	{
		return AvatarActor && TileManagerSubsystem && SourceTile && TargetingIntent.HitTile;
	}
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

	/** 선택 가능 타일과 적용될 대상이 될 수 있는 후보 타일을 모두 Out 인자로 반환합니다. */
	virtual void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 시전 결과에 포함되는 타일과 유효한 대상 Actor를 Out 인자로 반환합니다. */
	virtual void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const;

	/**
	 * 기본 구현은 GetTargetTiles를 그대로 호출합니다.
	 * 필요에 따라 하위 구조체가 오버라이드해 AI가 Player를 공격하기 위해 어떤 타일이 필요한지 계산합니다.
	 */
	virtual void GetTargetTilesForAI(FEffectTargetTileSelectorContext& Context) const;

protected:
	/** 시전 가능 범위에 해당하는 타일들을 Out 인자로 반환합니다. */
	virtual void GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 시전 시 적용 범위에 해당하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 수집된 Target들 중 중복 대상을 제거하고 TargetTeamRelation 규칙에 일치하는 대상만 남도록 걸러냅니다. */
	void ResolveTargetActors(FEffectTargetTileSelectorContext& Context) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	ETargetTeamRelation TargetTeamRelation = ETargetTeamRelation::OpposingTeam;
};
