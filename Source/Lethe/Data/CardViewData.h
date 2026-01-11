// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "CardViewData.generated.h"

USTRUCT(BlueprintType)
struct FCardViewInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> CardTexture;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText CardNameText;
	
	// 아래 Text는 런타임 중 Ability를 참조해 동적으로 채워집니다.
	FText CardDescriptionText;
};

UCLASS()
class LETHE_API UCardViewData : public UDataAsset
{
	GENERATED_BODY()

public:
	FCardViewInfo* FindCardInfoByTag(const FGameplayTag& InAbilityTag);

	FVector2D GetCardSize() const;
	float GetCardHighlightScale() const;

protected:
	// Key는 AbilityTag, Value는 Card의 View를 초기화하는 데에 필요한 에셋들을 묶은 구조체입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FCardViewInfo> CardViewData;

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	FVector2D CardSize = FVector2D(120.f, 168.f);

	UPROPERTY(EditDefaultsOnly, Category = "Card")
	float CardHighlightScale = 2.f;
};
