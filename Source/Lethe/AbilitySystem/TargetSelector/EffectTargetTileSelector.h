// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EffectTargetTileSelector.generated.h"

class ATile;

UCLASS(Abstract, NotBlueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LETHE_API UEffectTargetTileSelector : public UObject
{
	GENERATED_BODY()

public:
	virtual void Select(const AActor* AvatarActor, const ATile* CenterTile, TArray<ATile*>& OutTiles);
};
