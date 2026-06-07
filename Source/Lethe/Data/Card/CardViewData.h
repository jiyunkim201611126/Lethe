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
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FVector2D CardSize = FVector2D(120.f, 168.f);

	/** Key는 CardTypeTag입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TMap<FGameplayTag, FLinearColor> CardTypeColors;
};
