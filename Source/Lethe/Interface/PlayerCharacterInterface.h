// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayerCharacterInterface.generated.h"

struct FGameplayTag;

UINTERFACE()
class UPlayerCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API IPlayerCharacterInterface
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetCharacterTag() = 0;
};
