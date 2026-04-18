// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/SaveGame/SavedCardTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "CardDataLoadSubsystem.generated.h"

class UCardDefinitionData;
class UCardSelfViewData;
class UCharacterDefinitionData;
class UPrimaryDataAsset;
struct FSavedCharacterDeck;

DECLARE_DELEGATE_OneParam(FOnPrimaryDataAssetsLoaded, const TArray<UPrimaryDataAsset*>&)
DECLARE_DELEGATE_ThreeParams(FOnAllCardDataLoaded, const FGameplayTag& /* CharacterTag */, const TArray<FLoadedCardInfo>& /* LoadedCards */, const bool /* bEquipped */);

UENUM()
enum class ECardDataAssetType : uint8
{
	CardDefinition,
	CardSelfView,
	CharacterDefinition
};

USTRUCT(BlueprintType)
struct FLoadedCardInfo
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UCardDefinitionData> CardDefinition = nullptr;

	UPROPERTY()
	TObjectPtr<UCardSelfViewData> SelfViewData = nullptr;

	UPROPERTY()
	TObjectPtr<UCharacterDefinitionData> CharacterDefinition = nullptr;

	FSavedCard SavedCardInfo;
};

USTRUCT()
struct FPendingCardDataLoadRequest
{
	GENERATED_BODY()

	FGameplayTag CharacterTag;
	uint8 bEquipped : 1 = false;

	FOnAllCardDataLoaded OnLoadedCallback;

	TArray<FSavedCard> LoadRequestedCards;

	UPROPERTY()
	TArray<TObjectPtr<UCardDefinitionData>> LoadedCardDefinitions;

	UPROPERTY()
	TArray<TObjectPtr<UCardSelfViewData>> LoadedCardViews;

	UPROPERTY()
	TArray<TObjectPtr<UCharacterDefinitionData>> LoadedCharacterDefinitions;

	UPROPERTY()
	TArray<FLoadedCardInfo> LoadedCardInfos;

	uint8 bCardDefinitionsLoaded : 1 = false;
	uint8 bCardViewsLoaded : 1 = false;
	uint8 bCharacterDefinitionsLoaded : 1 = false;
};

/**
 * AssetManager를 통해 카드용 PrimaryDataAsset들을 로드하는 Subsystem입니다.
 * 여기서 로드는 세이브 로드가 아닌, 런타임 중 메모리에 올리는 에셋 로드입니다.
 * LetheAssetManager가 캐싱한 에셋 식별 정보를 사용해 CardDefinition, CardSelfView, CharacterDefinition Data Asset들을 비동기 로드해서 콜백으로 돌려줍니다.
 * 이를 통해 프로젝트에 수많은 카드가 존재해도, 런타임에 필요한 카드 관련 에셋만 메모리에 올려서 사용할 수 있습니다.
 *
 * 결과적으로 담당하는 역할은, PrimaryDataAsset 로드 요청이 오면 호출부가 사용하기 좋은 단위의 비동기 콜백으로 반환하는 것입니다.
 */
UCLASS()
class LETHE_API UCardDataLoadSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End of USubsystem Interface

	/** 캐릭터의 카드 덱을 모두 로드합니다. */
	void LoadCardData(const FGameplayTag& CharacterTag, const TArray<FSavedCard>& Cards, bool bEquipped, const FOnAllCardDataLoaded& OnLoadedCallback);

	/** GameplayTag 배열과 로드할 데이터 타입을 받아 PrimaryDataAsset 로드를 요청합니다. */
	void LoadPrimaryDataAssets(const TArray<FGameplayTag>& InGameplayTags, const ECardDataAssetType AssetType, FOnPrimaryDataAssetsLoaded OnComplete) const;

	void ChangeCharacterDecksKeyToSave(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, TMap<uint64, FSavedCharacterDeck>& OutDecks) const;
	void ChangeCharacterDecksKeyToLoad(const TMap<uint64, FSavedCharacterDeck>& InDecks, TMap<FGameplayTag, FSavedCharacterDeck>& OutDecks) const;

private:
	void OnPrimaryDataAssetsLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnPrimaryDataAssetsLoaded& OnComplete) const;
	bool TryGetPrimaryAssetId(const FGameplayTag& GameplayTag, const ECardDataAssetType AssetType, FPrimaryAssetId& OutAssetId) const;

	void OnCardDefinitionsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedDefinitions);
	void OnCardViewsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedCardViews);
	void OnCharacterDefinitionsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedCharacterDefinitions);
	void TryFinishCardDataLoad(const uint64 RequestId);
	
	void FillCardTagInSavedCardStruct(FSavedCharacterDeck& OutDeck) const;

private:
	uint64 NextCardDataLoadRequestId = 1;

	UPROPERTY()
	TMap<uint64, FPendingCardDataLoadRequest> PendingCardDataLoadRequests;
};
