// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

UINTERFACE(NotBlueprintable)
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API ICombatInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual void SetLocationOnTile(FVector InTileLocation) = 0;
	virtual void Die() = 0;
};
