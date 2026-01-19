// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "DeckEditingCardListObject.generated.h"

struct FCardSelfViewInfo;

UCLASS()
class LETHE_API UDeckEditingCardListObject : public UObject
{
	GENERATED_BODY()

public:
	FGameplayTag CardTag;
	FGameplayTag CharacterTag;

	FCardSelfViewInfo* CardSelfViewInfo;
	FLinearColor* CardTypeColor;
};
