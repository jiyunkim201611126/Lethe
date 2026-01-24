// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/SaveGame/DeckSaveGame.h"
#include "UObject/Object.h"
#include "DeckEditingCardListObject.generated.h"

UCLASS()
class LETHE_API UDeckEditingCardListObject : public UObject
{
	GENERATED_BODY()

public:
	FSavedCard CardInfo;
	
	FLinearColor* CardTypeColor;
	
	UPROPERTY()
	TObjectPtr<UTexture2D> CardTexture;
};
