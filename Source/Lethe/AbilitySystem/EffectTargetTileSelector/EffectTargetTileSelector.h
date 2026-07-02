// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/TileData.h"
#include "UObject/Object.h"
#include "EffectTargetTileSelector.generated.h"

class APlayerController;
class ATile;

UCLASS(Abstract, NotBlueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LETHE_API UEffectTargetTileSelector : public UObject
{
	GENERATED_BODY()

public:
	virtual void GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles);

	/** 시전 시 적용될 대상이 존재하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const;

protected:
	/** 시전 가능 범위에 해당하는 타일들을 Out 인자로 반환합니다. */
	virtual void GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const;

	/** 시전 시 적용 범위에 해당하는 타일들을 모두 Out 인자로 반환합니다. */
	virtual void GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const;
};
