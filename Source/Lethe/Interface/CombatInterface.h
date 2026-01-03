// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "CombatInterface.generated.h"

UINTERFACE()
class UCombatInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API ICombatInterface
{
	GENERATED_BODY()

public:
	virtual void Die() = 0;
};
