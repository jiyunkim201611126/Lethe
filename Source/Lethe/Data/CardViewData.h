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

	// 아래 Text는 런타임 중 Ability를 참조해 동적으로 채워집니다.
	FText CardNameText;
	FText CardDescriptionText;
};

UCLASS()
class LETHE_API UCardViewData : public UDataAsset
{
	GENERATED_BODY()

public:
	FCardViewInfo* FindCardInfoByTag(const FGameplayTag& InAbilityTag);

	/**
	 * 이 변수는 핸드에 마우스를 올린 상태, Highlight 시점의 크기를 결정한다는 걸 명심하시길 바랍니다.
	 * 위젯 Transform의 RenderScale을 사용해서 확대/축소를 해야 CPU가 저렴하게 먹히는데, Scale을 1.0보다 크게 늘리면 글씨가 깨져버립니다.
	 * 따라서 HighlightSize를 1.0으로 기준을 잡고, UnhighlightSize는 CardPanelWidget에서 결정합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FVector2D CardHighlightSize;

protected:
	// Key는 AbilityTag, Value는 Card의 View를 초기화하는 데에 필요한 에셋들을 묶은 구조체입니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TMap<FGameplayTag, FCardViewInfo> CardViewData;
};
