// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "UObject/Object.h"
#include "CardWidgetInitContext.generated.h"

UCLASS(NotBlueprintable, NotBlueprintType)
class UCardWidgetInitContext : public UObject
{
	GENERATED_BODY()

public:
	FText CardNameText;

	FSavedCard SavedCard;

	UPROPERTY()
	UTexture2D* CardTexture = nullptr;

	FLinearColor CardTypeColor;

	/** DeckEditing 시점에 사용하는 '카드 무게' 표시 용도 변수입니다. */
	float Weight = 0.f;
};
