// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectTargetTileSelector.h"
#include "Lethe/Data/Stage/TileData.h"
#include "BFSTargetTileSelector.generated.h"

UCLASS()
class LETHE_API UBFSTargetTileSelector : public UEffectTargetTileSelector
{
	GENERATED_BODY()

public:
	virtual void Select(const AActor* AvatarActor, const ATile* CenterTile, TArray<AActor*>& OutTiles) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	FBFSRange Range;
};
