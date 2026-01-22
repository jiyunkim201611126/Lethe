// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PlayableCharacterInterface.generated.h"

struct FGameplayTag;

UINTERFACE()
class UPlayableCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API IPlayableCharacterInterface
{
	GENERATED_BODY()

public:
	virtual FGameplayTag GetCharacterTag() const = 0;
};
