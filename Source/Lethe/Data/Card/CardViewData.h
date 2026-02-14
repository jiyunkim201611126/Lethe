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
	FVector2D GetCardSize() const;
	FVector2D GetCardImageSize() const;
	float GetCardExpandScale() const;

	FLinearColor* FindCardTypeColor(const FGameplayTag& InCardTypeTag);

protected:
	// 4픽셀의 Outline을 포함한 수치입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FVector2D CardSize = FVector2D(124.f, 172.f);

	// 카드 일러스트 사이즈입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FVector2D CardImageSize = FVector2D(104.f, 104.f);

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float CardExpandScale = 2.f;

	// Key는 CardTypeTag입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TMap<FGameplayTag, FLinearColor> CardTypeColors;
};
