// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "UObject/Object.h"
#include "DeckEditingCardListObject.generated.h"

UCLASS()
class LETHE_API UDeckEditingCardListObject : public UObject
{
	GENERATED_BODY()

public:
	FSavedCard SavedCardInfo;
	
	FLinearColor CardTypeColor;
	
	UPROPERTY()
	TObjectPtr<UTexture2D> CardTexture;

	int32 Weight = 0;
};
