// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CardViewData.generated.h"

UCLASS()
class LETHE_API UCardViewData : public UDataAsset
{
	GENERATED_BODY()

public:
	FLinearColor GetCardTypeColor(const FGameplayTag& InCardTypeTag) const;

public:
	/** Key는 CardTypeTag입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TMap<FGameplayTag, FLinearColor> CardTypeColors;
};
