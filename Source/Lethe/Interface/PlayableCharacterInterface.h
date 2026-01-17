// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayableCharacterInterface.generated.h"

UINTERFACE()
class UPlayableCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API IPlayableCharacterInterface
{
	GENERATED_BODY()

public:
	virtual FColor GetCardFrontsideColor() = 0;
	virtual FColor GetCardBacksideColor() = 0;
	virtual FGameplayTag GetCharacterTag() = 0;
};
