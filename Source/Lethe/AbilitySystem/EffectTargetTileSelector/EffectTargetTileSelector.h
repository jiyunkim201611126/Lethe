// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TargetTileSelectData.h"
#include "EffectTargetTileSelector.generated.h"

class APlayerController;
class ATile;
class UTileManagerSubsystem;
struct FTargetSelectResult;

UENUM(BlueprintType)
enum class ETargetTeamRelation : uint8
{
	AllSides,
	SameTeam,
	OpposingTeam,
};

struct FEffectTargetTileSelectorContext
{
	const AActor* AvatarActor = nullptr;
	const UTileManagerSubsystem* TileManagerSubsystem = nullptr;
	const ATile* CurrentTile = nullptr;

	FTargetingIntent TargetingIntent;

	TArray<ATile*> OutSelectCandidateTiles;
	TArray<FTargetSelectResult> OutTargetTileResults;

	bool IsValid() const
	{
		return AvatarActor && TileManagerSubsystem && CurrentTile && TargetingIntent.HitTile;
	}
};

/**
 * Ability 발동 시 Effect가 적용될 수 있는 후보 타일을 모두 Out 인자에 추가합니다.
 * FSelectedTarget의 TargetTile은 반드시 추가되며, 범위가 맵 바깥을 벗어난 경우 nullptr이 추가됩니다.
 * FSelectedTarget의 ActorOnTile은 실제로 Effect가 적용될 대상만 추가됩니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FEffectTargetTileSelector
{
	GENERATED_BODY()

public:
	virtual ~FEffectTargetTileSelector() = default;

	/** 선택 가능 타일과 적용될 대상이 될 수 있는 후보 타일을 모두 Out 인자로 반환합니다. */
	virtual void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 시전 시 실제로 적용될 대상이 존재하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const;

protected:
	/** 시전 가능 범위에 해당하는 타일들을 Out 인자로 반환합니다. */
	virtual void GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 시전 시 적용 범위에 해당하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	void ResolveTargetActors(FEffectTargetTileSelectorContext& Context) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	ETargetTeamRelation TargetTeamRelation = ETargetTeamRelation::OpposingTeam;
};
