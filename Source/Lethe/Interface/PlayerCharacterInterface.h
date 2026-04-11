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

	virtual void SetPersonalColor(const FLinearColor& InColor);
	virtual const FLinearColor& GetPersonalColor() const = 0;

	virtual void SetPlayerOrderIndex(const int32 Index) = 0;
	virtual int32 GetPlayerOrderIndex() const = 0;
};
