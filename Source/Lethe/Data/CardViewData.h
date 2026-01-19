// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CardViewData.generated.h"

// 카드 이름이나 일러스트 등 '자신'의 View를 초기화하는 데에 사용되는 데이터 묶음 구조체입니다.
USTRUCT(BlueprintType)
struct FCardSelfViewInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText CardNameText;
	
	// 아래 Text는 런타임 중 Ability를 참조해 동적으로 채워집니다.
	FText CardDescriptionText;
};

// 카드의 '주인'이 누구인지를 구분할 수 있는 View를 초기화하는 데에 사용되는 데이터 묶음 구조체입니다.
USTRUCT(BlueprintType)
struct FCardOwnerViewInfo
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FColor CardFrontsideColor;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Card")
	FColor CardBacksideColor;
};

UCLASS()
class LETHE_API UCardViewData : public UDataAsset
{
	GENERATED_BODY()

public:
	FCardSelfViewInfo* FindCardSelfViewInfoByTag(const FGameplayTag& InCardTag);
	FCardOwnerViewInfo* FindCardOwnerViewInfoByTag(const FGameplayTag& InCharacterTag);

	FVector2D GetCardSize() const;
	float GetCardHighlightScale() const;

	FLinearColor* FindCardTypeColor(const FGameplayTag& InCardTypeTag);

protected:
	// Key는 CardTag입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FCardSelfViewInfo> CardSelfViewData;

	// Key는 CharacterTag입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FCardOwnerViewInfo> CardOwnerViewData;

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FVector2D CardSize = FVector2D(120.f, 168.f);

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float CardHighlightScale = 2.f;

	// Key는 CardTypeTag입니다.
	UPROPERTY(EditDefaultsOnly, Category = "Card")
	TMap<FGameplayTag, FLinearColor> CardTypeColors;
};
