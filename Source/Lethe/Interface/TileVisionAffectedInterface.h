// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TileVisionAffectedInterface.generated.h"

class ATile;

UINTERFACE(Blueprintable)
class UTileVisionAffectedInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API ITileVisionAffectedInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent)
	void UpdateHiddenByTile(const ATile* Tile);
};
