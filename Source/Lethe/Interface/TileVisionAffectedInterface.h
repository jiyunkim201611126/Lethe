// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TileVisionAffectedInterface.generated.h"

class ATile;

UINTERFACE()
class UTileVisionAffectedInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API ITileVisionAffectedInterface
{
	GENERATED_BODY()

public:
	virtual void UpdateHiddenByTile(const ATile* Tile) = 0;
};
