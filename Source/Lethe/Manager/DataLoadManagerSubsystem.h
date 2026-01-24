// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DataLoadManagerSubsystem.generated.h"

struct FSavedCharacterDeck;
class UCardDefinitionData;
class UCardSelfViewData;
class UCharacterDefinitionData;

DECLARE_DELEGATE_OneParam(FOnCardDefinitionsLoaded, const TArray<UCardDefinitionData*>&)
DECLARE_DELEGATE_TwoParams(FOnCardViewLoaded, const UCardSelfViewData*, const UCharacterDefinitionData*)

/**
 * AssetManager를 통해 카드용 PrimaryDataAsset들을 로드하는 Subsystem입니다.
 * 여기서 로드는 세이브 로드가 아닌, 런타임 중 메모리에 올리는 에셋 로드입니다.
 * 게임 시작 시 식별용 태그인 CardTag, CharacterTag와 PrimaryAssetId를 매핑합니다.
 * 그 후 요청에 다라 CardDefinition, CardSelfView, CharacterDefinition Data Asset들을 비동기 로드해서 콜백으로 돌려줍니다.
 * 이를 통해 프로젝트에 수많은 카드가 존재해도, 런타임에 필요한 카드 관련 에셋만 메모리에 올려서 사용할 수 있습니다.
 */
UCLASS()
class LETHE_API UDataLoadManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void LoadCardDefinitionData(const TArray<FGameplayTag>& InCardTags, FOnCardDefinitionsLoaded OnComplete);
	void LoadCardViewData(const FGameplayTag& InCardTag, const FGameplayTag& InCharacterTag, FOnCardViewLoaded OnComplete);

	void ChangeCharacterDecksKeyToSave(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, TMap<uint64, FSavedCharacterDeck>& OutDecks) const;
	void ChangeCharacterDecksKeyToLoad(const TMap<uint64, FSavedCharacterDeck>& InDecks, TMap<FGameplayTag, FSavedCharacterDeck>& OutDecks) const;
	
	FGameplayTag GetCharacterTagById(const uint64 InCharacterId) const;

private:
	void OnCardDefinitionDataLoaded(const TArray<FPrimaryAssetId>& AssetsToLoad, const FOnCardDefinitionsLoaded& OnComplete) const;
	void OnCardViewDataLoaded(const FPrimaryAssetId& SelfViewId, const FPrimaryAssetId& CharacterDefinitionId, const FOnCardViewLoaded& OnComplete) const;
	
	void GetCharacterMappingCaches(TMap<FGameplayTag, uint64>& OutTagToIds, TMap<uint64, FGameplayTag>& OutIdToTags) const;
	void FillCardTagInSavedCardStruct(FSavedCharacterDeck& OutDeck) const;

private:
	// Card, Character의 Tag를 Key, PrimaryAssetId를 Value로 하는 TMap입니다.
	TMap<FGameplayTag, FPrimaryAssetId> CardDefinitionDataAssetIds;
	TMap<FGameplayTag, FPrimaryAssetId> CardSelfViewDataAssetIds;
	TMap<FGameplayTag, FPrimaryAssetId> CharacterDefinitionDataAssetIds;

	// CardId를 Key, CardTag를 Value로 하는 TMap입니다.
	TMap<uint64, FGameplayTag> CardIdToTags;
};
