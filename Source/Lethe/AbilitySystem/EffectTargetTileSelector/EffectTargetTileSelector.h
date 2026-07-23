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

struct FEffectTargetTileSelectorContext
{
	const AActor* AvatarActor = nullptr;
	const UTileManagerSubsystem* TileManagerSubsystem = nullptr;
	const ATile* CurrentTile = nullptr;

	FTargetingIntent TargetingIntent;

	TArray<ATile*> OutSelectCandidateTiles;
	TArray<FTargetSelectionResult> OutTargetResults;

	bool IsValid() const
	{
		return AvatarActor && TileManagerSubsystem && CurrentTile && TargetingIntent.HitTile;
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
