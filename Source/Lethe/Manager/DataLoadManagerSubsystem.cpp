// Copyright JETBLU, Inc. All Rights Reserved.

#include "DataLoadManagerSubsystem.h"
#include "Engine/AssetManager.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardSelfViewData.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/SaveGame/DeckSaveGame.h"

void UDataLoadManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UAssetManager& AssetManager = UAssetManager::Get();
	
	// CardDefinition 타입의 모든 에셋 메타데이터를 가져옵니다. 로드하는 과정이 아닙니다.
	TArray<FAssetData> CardDefinitionAssetDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardDefinition")), CardDefinitionAssetDataList);
	for (const FAssetData& CardDefinitionAssetData : CardDefinitionAssetDataList)
	{
		// 찾은 메타데이터에서 CardId를 가져와 PrimaryAssetId와 매핑합니다.
		FString FoundCardIdString;
		FString FoundCardTagString;
		CardDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardDefinitionData, CardId), FoundCardIdString);
		CardDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardDefinitionData, CardTag), FoundCardTagString);

		if (!FoundCardIdString.IsEmpty() && !FoundCardTagString.IsEmpty())
		{
			uint64 CardId = 0;
			if (!LexTryParseString(CardId, *FoundCardIdString))
			{
				continue;
			}
			FGameplayTag CardTag;
			CardTag.FromExportString(FoundCardTagString);
			FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForData(CardDefinitionAssetData);
			
			checkf(!CardDefinitionDataAssetIds.Contains(CardTag), TEXT("CardId: %d가 중복인 CardDefinition Data Asset이 존재합니다."), CardId);
			
			CardDefinitionDataAssetIds.Emplace(CardTag, AssetId);

			// 카드는 캐릭터에 비해 매우 많으므로, O(1)으로 찾을 수 없는 AssetData 특성상 미리 매핑해둡니다.
			CardIdToTags.Emplace(CardId, CardTag);
		}
	}

	TArray<FAssetData> CardSelfViewAssetDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardSelfView")), CardSelfViewAssetDataList);
	for (const FAssetData& CardSelfViewAssetData : CardSelfViewAssetDataList)
	{
		FString FoundCardIdString;
		if (CardSelfViewAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardSelfViewData, CardId), FoundCardIdString))
		{
			uint64 CardId = 0;
			if (!LexTryParseString(CardId, *FoundCardIdString))
			{
				continue;
			}
			if (const FGameplayTag* CardTag = CardIdToTags.Find(CardId))
			{
				FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForData(CardSelfViewAssetData);

				checkf(!CardSelfViewDataAssetIds.Contains(*CardTag), TEXT("CardId: %d가 중복인 CardDefinition Data Asset이 존재합니다."), CardId);

				CardSelfViewDataAssetIds.Emplace(*CardTag, AssetId);
			}
			else
			{
				checkf(false, TEXT("CardId: %d가 일치하지 않는 CardDefinition과 CardSelfView Data Asset이 존재합니다."), CardId)
			}
		}
	}

	TArray<FAssetData> CharacterDefinitionDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CharacterDefinition")), CharacterDefinitionDataList);
	for (const FAssetData& CharacterDefinitionData : CharacterDefinitionDataList)
	{
		FString FoundCharacterTagString;
		if (CharacterDefinitionData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCharacterDefinitionData, CharacterTag), FoundCharacterTagString))
		{
			FGameplayTag CharacterTag;
			CharacterTag.FromExportString(FoundCharacterTagString);
			FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForData(CharacterDefinitionData);

			checkf(!CharacterDefinitionDataAssetIds.Contains(CharacterTag), TEXT("CharacterTag: %s가 중복인 CharacterDefinition Data Asset이 존재합니다."), *CharacterTag.GetTagName().ToString());

			CharacterDefinitionDataAssetIds.Emplace(CharacterTag, AssetId);
		}
	}
}

void UDataLoadManagerSubsystem::AddLoader(UObject* Loader)
{
	ActivatedLoaders.Emplace(Loader);
}

void UDataLoadManagerSubsystem::RemoveLoader(UObject* Loader)
{
	ActivatedLoaders.Remove(Loader);
}

void UDataLoadManagerSubsystem::LoadCardDefinitionData(const TArray<FGameplayTag>& InCardTags, FOnCardDefinitionsLoaded OnComplete)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	// 매개변수로 받은 CardTag로 로드할 PrimaryDataAsset의 Id를 가져옵니다.
	for (const FGameplayTag& CardTag : InCardTags)
	{
		if (CardDefinitionDataAssetIds.Contains(CardTag))
		{
			AssetsToLoad.Emplace(CardDefinitionDataAssetIds[CardTag]);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CardTag: %s에 해당하는 CardDefinition DataAsset이 없습니다."), *CardTag.GetTagName().ToString());
		}
	}
	
	if (!AssetsToLoad.IsEmpty())
	{
		// 로드할 객체가 있다면 로드를 시작합니다.
		AssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, AssetsToLoad, OnComplete]()
		{
			OnCardDefinitionDataLoaded(AssetsToLoad, OnComplete);
		}));
	}
	else
	{
		OnComplete.ExecuteIfBound(TArray<UCardDefinitionData*>());
	}
}

void UDataLoadManagerSubsystem::OnCardDefinitionDataLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnCardDefinitionsLoaded& OnComplete) const
{
	const UAssetManager& AssetManager = UAssetManager::Get();
			
	TArray<UCardDefinitionData*> LoadedAssets;

	for (const FPrimaryAssetId& Id : LoadedAssetsId)
	{
		// 로드가 완료되면 해당 객체를 실제로 가져옵니다.
		if (UCardDefinitionData* LoadedAsset = Cast<UCardDefinitionData>(AssetManager.GetPrimaryAssetObject(Id)))
		{
			LoadedAssets.Emplace(LoadedAsset);
		}
	}

	// 로드된 객체를 콜백으로 반환합니다.
	OnComplete.ExecuteIfBound(LoadedAssets);
}

void UDataLoadManagerSubsystem::LoadCardViewData(const FGameplayTag& InCardTag, const FGameplayTag& InCharacterTag, FOnCardViewLoaded OnComplete)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	FPrimaryAssetId SelfViewId = CardSelfViewDataAssetIds.Contains(InCardTag) ? CardSelfViewDataAssetIds[InCardTag] : FPrimaryAssetId();
	FPrimaryAssetId CharacterDefinitionId = CharacterDefinitionDataAssetIds.Contains(InCharacterTag) ? CharacterDefinitionDataAssetIds[InCharacterTag] : FPrimaryAssetId();

	if (SelfViewId.IsValid() && CharacterDefinitionId.IsValid())
	{
		AssetsToLoad.Emplace(SelfViewId);
		AssetsToLoad.Emplace(CharacterDefinitionId);
		
		AssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, SelfViewId, CharacterDefinitionId, OnComplete]()
		{
			OnCardViewDataLoaded(SelfViewId, CharacterDefinitionId, OnComplete);
		}));
	}
	else
	{
		OnComplete.ExecuteIfBound(nullptr, nullptr);
		UE_LOG(LogTemp, Error, TEXT("CardTag: %s, 혹은 CharacterTag: %s에 해당하는 CardViewData DataAsset이 없습니다."), *InCardTag.GetTagName().ToString(), *InCharacterTag.GetTagName().ToString());
	}
}

void UDataLoadManagerSubsystem::LoadCharacterDefinitionData(const TArray<FGameplayTag>& InCharacterTags, FOnCharacterDefinitionsLoaded OnComplete)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	for (const FGameplayTag& CharacterTag : InCharacterTags)
	{
		if (CharacterDefinitionDataAssetIds.Contains(CharacterTag))
		{
			AssetsToLoad.Emplace(CharacterDefinitionDataAssetIds[CharacterTag]);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CharacterTag: %s에 해당하는 CharacterDefinition DataAsset이 없습니다."), *CharacterTag.GetTagName().ToString());
		}
	}
	
	if (!AssetsToLoad.IsEmpty())
	{
		// 로드할 객체가 있다면 로드를 시작합니다.
		AssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, AssetsToLoad, OnComplete]()
		{
			OnCharacterDefinitionDataLoaded(AssetsToLoad, OnComplete);
		}));
	}
	else
	{
		OnComplete.ExecuteIfBound(TArray<UCharacterDefinitionData*>());
	}
}

void UDataLoadManagerSubsystem::OnCharacterDefinitionDataLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnCharacterDefinitionsLoaded& OnComplete) const
{
	const UAssetManager& AssetManager = UAssetManager::Get();
			
	TArray<UCharacterDefinitionData*> LoadedAssets;

	for (const FPrimaryAssetId& Id : LoadedAssetsId)
	{
		// 로드가 완료되면 해당 객체를 실제로 가져옵니다.
		if (UCharacterDefinitionData* LoadedAsset = Cast<UCharacterDefinitionData>(AssetManager.GetPrimaryAssetObject(Id)))
		{
			LoadedAssets.Emplace(LoadedAsset);
		}
	}

	// 로드된 객체를 콜백으로 반환합니다.
	OnComplete.ExecuteIfBound(LoadedAssets);
}

void UDataLoadManagerSubsystem::OnCardViewDataLoaded(const FPrimaryAssetId& SelfViewId, const FPrimaryAssetId& CharacterDefinitionId, const FOnCardViewLoaded& OnComplete) const
{
	const UAssetManager& AssetManager = UAssetManager::Get();
	
	UCardSelfViewData* SelfViewData = Cast<UCardSelfViewData>(AssetManager.GetPrimaryAssetObject(SelfViewId));
	UCharacterDefinitionData* CharacterDefinitionData = Cast<UCharacterDefinitionData>(AssetManager.GetPrimaryAssetObject(CharacterDefinitionId));
	
	OnComplete.ExecuteIfBound(SelfViewData, CharacterDefinitionData);
}

void UDataLoadManagerSubsystem::ChangeCharacterDecksKeyToSave(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, TMap<uint64, FSavedCharacterDeck>& OutDecks) const
{
	// CharacterTag를 Key로 사용하던 Deck을 세이브하기 위해 CharacterId로 교체합니다.
	TMap<FGameplayTag, uint64> TagToId;
	TMap<uint64, FGameplayTag> Unused;
	GetCharacterMappingCaches(TagToId, Unused);

	for (const auto& Deck : InDecks)
	{
		if (uint64* FoundId = TagToId.Find(Deck.Key))
		{
			OutDecks.Emplace(*FoundId, Deck.Value);
		}
	}

	// CardTag는 저장되진 않지만, 게임 첫 시작인 경우에 대응하기 위해 여기서도 CardTag를 할당해줍니다.
	for (auto& Deck : OutDecks)
	{
		FillCardTagInSavedCardStruct(Deck.Value);
	}
}

void UDataLoadManagerSubsystem::ChangeCharacterDecksKeyToLoad(const TMap<uint64, FSavedCharacterDeck>& InDecks, TMap<FGameplayTag, FSavedCharacterDeck>& OutDecks) const
{
	// CharacterId로 세이브했던 Deck의 Key를 CharacterTag로 교체해 캐싱합니다.
	TMap<FGameplayTag, uint64> Unused;
	TMap<uint64, FGameplayTag> IdToTag;
	GetCharacterMappingCaches(Unused, IdToTag);

	for (const auto& Deck : InDecks)
	{
		if (FGameplayTag* FoundTag = IdToTag.Find(Deck.Key))
		{
			OutDecks.Emplace(*FoundTag, Deck.Value);
		}
	}

	// 로드된 카드를 내부에 CardTag를 채워넣습니다.
	for (auto& Deck : OutDecks)
	{
		FillCardTagInSavedCardStruct(Deck.Value);
	}
}

void UDataLoadManagerSubsystem::GetCharacterMappingCaches(TMap<FGameplayTag, uint64>& OutTagToIds, TMap<uint64, FGameplayTag>& OutIdToTags) const
{
	// CharacterDefinition Data에서 Tag와 Id를 가져와 TMap으로 양방향 매핑해 반환합니다.
	const UAssetManager& AssetManager = UAssetManager::Get();
	
	TArray<FAssetData> CharacterDefinitionAssetDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CharacterDefinition")), CharacterDefinitionAssetDataList);

	for (const FAssetData& CharacterDefinitionAssetData : CharacterDefinitionAssetDataList)
	{
		FString FoundTagString;
		FString FoundIdString;
		CharacterDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCharacterDefinitionData, CharacterTag), FoundTagString);
		CharacterDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCharacterDefinitionData, CharacterId), FoundIdString);

		if (!FoundTagString.IsEmpty() && !FoundIdString.IsEmpty())
		{
			FGameplayTag FoundTag;
			FoundTag.FromExportString(FoundTagString);
			uint64 FoundId = 0;
			if (!LexTryParseString(FoundId, *FoundIdString))
			{
				continue;
			}

			OutTagToIds.Emplace(FoundTag, FoundId);
			OutIdToTags.Emplace(FoundId, FoundTag);
		}
	}
}

void UDataLoadManagerSubsystem::FillCardTagInSavedCardStruct(FSavedCharacterDeck& OutDeck) const
{
	for (FSavedCard& SavedCard : OutDeck.Deck)
	{
		if (const FGameplayTag* CardTag = CardIdToTags.Find(SavedCard.CardId))
		{
			SavedCard.CardTag = *CardTag;
		}
	}
}

FGameplayTag UDataLoadManagerSubsystem::GetCharacterTagById(const uint64 InCharacterId) const
{
	// 캐릭터는 카드와 달리 그리 많지 않으므로, 필요할 때 AssetData를 순회해 가져옵니다.
	// 캐릭터 수가 많아져도 실제로 플레이에 사용되는 건 4명뿐이므로 이와 같은 로직을 사용합니다.
	TMap<FGameplayTag, uint64> Unused;
	TMap<uint64, FGameplayTag> IdToTag;
	GetCharacterMappingCaches(Unused, IdToTag);
	return IdToTag.FindRef(InCharacterId);
}
