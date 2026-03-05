// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheCharacterBase.h"
#include "EnemyCharacterBase.generated.h"

UCLASS()
class LETHE_API AEnemyCharacterBase : public ALetheCharacterBase
{
	GENERATED_BODY()

public:
	void SetEnemyAbilityPriority(const int32 InPriority) const;
};
