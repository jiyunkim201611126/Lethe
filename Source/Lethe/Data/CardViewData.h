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
};

UCLASS()
class LETHE_API UCardViewData : public UDataAsset
{
	GENERATED_BODY()

public:
	FCardViewInfo* FindCardInfoByTag(const FGameplayTag& InAbilityTag);

protected:
	// Key는 AbilityTag, Value는 Card의 View를 초기화하는 데에 필요한 에셋들을 묶은 구조체입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FCardViewInfo> CardViewData;
};
