// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/TileData.h"
#include "EffectTargetTileSelector.generated.h"

class UTileManagerSubsystem;
class APlayerController;
struct FTargetTileResult;

struct FEffectTargetTileSelectorContext
{
	const AActor* AvatarActor = nullptr;
	const APlayerController* PlayerController = nullptr;
	const UTileManagerSubsystem* TileManagerSubsystem = nullptr;
	const ATile* CurrentTile = nullptr;

	TArray<ATile*> OutSelectCandidateTiles;
	TArray<FTargetTileResult> OutTargetTileResults;

	bool IsValid() const
	{
		return AvatarActor && PlayerController && TileManagerSubsystem && CurrentTile;
	}
};

/**
 * Ability 발동 시 Effect를 적용할 대상이 밟고 있는 타일을 가져옵니다.
 * 기본적으로 '유효한 대상'이 밟고 있는 타일만 추가하며, 유효하지 않은 대상이 있거나, 아무런 Actor도 없는 타일의 경우 nullptr을 추가합니다.
 * 즉, 최종적으로 TargetTiles 또한 TargetCandidateTiles와 같은 길이의 배열이 되며, 이 중 Effect를 적용하지 않을 대상은 nullptr로 들어갑니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FEffectTargetTileSelector
{
	GENERATED_BODY()

	virtual ~FEffectTargetTileSelector() = default;

	virtual void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 시전 시 적용될 대상이 존재하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const;

protected:
	/** 시전 가능 범위에 해당하는 타일들을 Out 인자로 반환합니다. */
	virtual void GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/** 시전 시 적용 범위에 해당하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;
};
