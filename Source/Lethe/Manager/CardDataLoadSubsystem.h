// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CardDataLoadSubsystem.generated.h"

class UCardDefinitionData;
class UCharacterDefinitionData;
class UPrimaryDataAsset;
struct FSavedCharacterDeck;

/** CardDefinitions와 SavedCardInfos는 순서대로 추가되므로, 같은 인덱스라면 같은 카드를 표현합니다. */
USTRUCT(BlueprintType)
struct FLoadedCardInfos
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UCharacterDefinitionData> CharacterDefinition = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UCardDefinitionData>> CardDefinitions;

	TArray<FSavedCard> SavedCardInfos;
};

DECLARE_DELEGATE_ThreeParams(FOnAllCardDataLoaded, const FGameplayTag& /* CharacterTag */, const FLoadedCardInfos& /* LoadedCardInfos */, const bool /* bEquipped */);

USTRUCT()
struct FPendingCardDataLoadRequest
{
	GENERATED_BODY()

	/** DataAsset 로드를 위한 데이터 묶음입니다. */
	FGameplayTag CharacterTag;
	TArray<FSavedCard> LoadRequestedCards;
	uint8 bEquipped : 1 = false;

	/** 로드 완료 후 호출될 콜백입니다. */
	FOnAllCardDataLoaded OnLoadedCallback;

	/** 로드가 완료된 데이터입니다. */
	UPROPERTY()
	TArray<TObjectPtr<UCardDefinitionData>> LoadedCardDefinitions;

	UPROPERTY()
	TArray<TObjectPtr<UCharacterDefinitionData>> LoadedCharacterDefinitions;

	uint8 bCardDefinitionsLoaded : 1 = false;
	uint8 bCharacterDefinitionLoaded : 1 = false;
};

/**
 * LetheAssetManager를 통해 카드 덱 데이터를 조립하는 매니저 클래스입니다.
 * 여기서 로드는 세이브 로드가 아닌, 런타임 중 메모리에 올리는 에셋 로드입니다.
 * LetheAssetManager가 캐싱한 에셋 식별 정보를 사용해 CardDefinition, CharacterDefinition Data Asset들을 비동기 로드해서 콜백으로 돌려줍니다.
 * 이를 통해 프로젝트에 수많은 카드가 존재해도, 런타임에 필요한 카드 관련 에셋만 메모리에 올려서 사용할 수 있습니다.
 */
UCLASS()
class LETHE_API UCardDataLoadSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface

	/** 캐릭터의 카드 덱을 모두 로드합니다. */
	void LoadCardData(const FGameplayTag& CharacterTag, const TArray<FSavedCard>& Cards, bool bEquipped, const FOnAllCardDataLoaded& OnLoadedCallback);

	void ChangeCharacterDecksKeyToSave(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, TMap<uint64, FSavedCharacterDeck>& OutDecks) const;
	void ChangeCharacterDecksKeyToLoad(const TMap<uint64, FSavedCharacterDeck>& InDecks, TMap<FGameplayTag, FSavedCharacterDeck>& OutDecks) const;

private:
	void OnCardDefinitionsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedDefinitions);
	void OnCharacterDefinitionsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedCharacterDefinitions);
	void TryFinishCardDataLoad(const uint64 RequestId);
	
	void FillCardTagInSavedCardStruct(FSavedCharacterDeck& OutDeck) const;

private:
	uint64 NextCardDataLoadRequestId = 1;

	UPROPERTY()
	TMap<uint64, FPendingCardDataLoadRequest> PendingCardDataLoadRequests;
};
