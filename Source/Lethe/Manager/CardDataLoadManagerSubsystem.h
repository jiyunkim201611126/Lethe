// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CardDataLoadManagerSubsystem.generated.h"

class UCardDefinitionData;
class UCardSelfViewData;
class UCardOwnerViewData;

DECLARE_DELEGATE_OneParam(FOnCardDefinitionsLoaded, const TArray<UCardDefinitionData*>&)
DECLARE_DELEGATE_TwoParams(FOnCardViewLoaded, const UCardSelfViewData*, const UCardOwnerViewData*)

/**
 * AssetManager를 통해 카드용 PrimaryDataAsset들을 로드하는 Subsystem입니다.
 * 게임 시작 시 식별용 태그인 CardTag, CharacterTag와 PrimaryAssetId를 매핑합니다.
 * 그 후 요청에 다라 CardDefinition, CardSelfView, CardOwnerView Data Asset들을 비동기 로드해서 콜백으로 돌려줍니다.
 */
UCLASS()
class LETHE_API UCardDataLoadManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void LoadCardDefinitionData(const TArray<FGameplayTag>& InCardTags, FOnCardDefinitionsLoaded OnComplete);
	void LoadCardViewData(const FGameplayTag& InCardTag, const FGameplayTag& InCharacterTag, FOnCardViewLoaded OnComplete);

private:
	void OnCardDefinitionDataLoaded(const TArray<FPrimaryAssetId>& AssetsToLoad, const FOnCardDefinitionsLoaded& OnComplete) const;
	void OnCardViewDataLoaded(const FPrimaryAssetId& SelfViewId, const FPrimaryAssetId& OwnerViewId, const FOnCardViewLoaded& OnComplete) const;

private:
	TMap<FGameplayTag, FPrimaryAssetId> CardDefinitionDataAssetIds;
	TMap<FGameplayTag, FPrimaryAssetId> CardSelfViewDataAssetIds;
	TMap<FGameplayTag, FPrimaryAssetId> CardOwnerViewDataAssetIds;
};
