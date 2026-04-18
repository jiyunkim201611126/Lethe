// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardDataLoadSubsystem.h"

#include "Engine/AssetManager.h"
#include "Lethe/Data/CardPrimaryDataAssetLoader.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardSelfViewData.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Manager/EngineSystem/LetheAssetManager.h"
#include "Lethe/SaveGame/SavedCardTypes.h"

void UCardDataLoadSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// AssetManager의 게임 시작 시점 초기화 관련한 함수가 딱히 적당한 게 없어 여기서 호출합니다.
	ULetheAssetManager::Get().BuildAssetIdCaches();
}

void UCardDataLoadSubsystem::Deinitialize()
{
	for (UObject* Loader : ActivatedLoaders)
	{
		if (UCardPrimaryDataAssetLoader* DataAssetLoader = Cast<UCardPrimaryDataAssetLoader>(Loader))
		{
			DataAssetLoader->Destruct();
		}
	}
	ActivatedLoaders.Empty();
	Super::Deinitialize();
}

void UCardDataLoadSubsystem::AddLoader(UObject* Loader)
{
	ActivatedLoaders.Emplace(Loader);
}

void UCardDataLoadSubsystem::RemoveLoader(UObject* Loader)
{
	ActivatedLoaders.Remove(Loader);
}

void UCardDataLoadSubsystem::LoadCardDefinitionData(const TArray<FGameplayTag>& InCardTags, FOnCardDefinitionsLoaded OnComplete) const
{
	ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	// 매개변수로 받은 CardTag로 로드할 PrimaryDataAsset의 Id를 가져옵니다.
	for (const FGameplayTag& CardTag : InCardTags)
	{
		FPrimaryAssetId CardDefinitionAssetId;
		if (LetheAssetManager.TryGetCardDefinitionAssetId(CardTag, CardDefinitionAssetId))
		{
			AssetsToLoad.Emplace(CardDefinitionAssetId);
		}
		else
		{
			checkf(false, TEXT("CardTag: %s에 해당하는 CardDefinition DataAsset이 없습니다."), *CardTag.GetTagName().ToString());
		}
	}
	
	if (!AssetsToLoad.IsEmpty())
	{
		// 로드할 객체가 있다면 로드를 시작합니다.
		LetheAssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, AssetsToLoad, OnComplete]()
		{
			OnCardDefinitionDataLoaded(AssetsToLoad, OnComplete);
		}));
	}
	else
	{
		OnComplete.ExecuteIfBound(TArray<UCardDefinitionData*>());
	}
}

void UCardDataLoadSubsystem::OnCardDefinitionDataLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnCardDefinitionsLoaded& OnComplete) const
{
	const ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();
	
	TArray<UCardDefinitionData*> LoadedAssets;

	for (const FPrimaryAssetId& Id : LoadedAssetsId)
	{
		// 로드가 완료되면 해당 객체를 실제로 가져옵니다.
		UCardDefinitionData* LoadedAsset = CastChecked<UCardDefinitionData>(LetheAssetManager.GetPrimaryAssetObject(Id));
		LoadedAssets.Emplace(LoadedAsset);
	}

	// 로드된 객체를 콜백으로 반환합니다.
	OnComplete.ExecuteIfBound(LoadedAssets);
}

void UCardDataLoadSubsystem::LoadCardViewData(const FGameplayTag& InCardTag, const FGameplayTag& InCharacterTag, FOnCardViewLoaded OnComplete) const
{
	ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	FPrimaryAssetId SelfViewId;
	FPrimaryAssetId CharacterDefinitionId;
	LetheAssetManager.TryGetCardSelfViewAssetId(InCardTag, SelfViewId);
	LetheAssetManager.TryGetCharacterDefinitionAssetId(InCharacterTag, CharacterDefinitionId);

	if (SelfViewId.IsValid() && CharacterDefinitionId.IsValid())
	{
		AssetsToLoad.Emplace(SelfViewId);
		AssetsToLoad.Emplace(CharacterDefinitionId);
		
		LetheAssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, SelfViewId, CharacterDefinitionId, OnComplete]()
		{
			OnCardViewDataLoaded(SelfViewId, CharacterDefinitionId, OnComplete);
		}));
	}
	else
	{
		checkf(false, TEXT("CardTag: %s, 혹은 CharacterTag: %s에 해당하는 CardViewData DataAsset이 없습니다."), *InCardTag.GetTagName().ToString(), *InCharacterTag.GetTagName().ToString());
	}
}

void UCardDataLoadSubsystem::LoadCharacterDefinitionData(const TArray<FGameplayTag>& InCharacterTags, FOnCharacterDefinitionsLoaded OnComplete) const
{
	ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	for (const FGameplayTag& CharacterTag : InCharacterTags)
	{
		FPrimaryAssetId CharacterDefinitionAssetId;
		if (LetheAssetManager.TryGetCharacterDefinitionAssetId(CharacterTag, CharacterDefinitionAssetId))
		{
			AssetsToLoad.Emplace(CharacterDefinitionAssetId);
		}
		else
		{
			checkf(false, TEXT("CharacterTag: %s에 해당하는 CharacterDefinition DataAsset이 없습니다."), *CharacterTag.GetTagName().ToString());
		}
	}
	
	if (!AssetsToLoad.IsEmpty())
	{
		// 로드할 객체가 있다면 로드를 시작합니다.
		LetheAssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, AssetsToLoad, OnComplete]()
		{
			OnCharacterDefinitionDataLoaded(AssetsToLoad, OnComplete);
		}));
	}
	else
	{
		OnComplete.ExecuteIfBound(TArray<UCharacterDefinitionData*>());
	}
}

void UCardDataLoadSubsystem::OnCharacterDefinitionDataLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnCharacterDefinitionsLoaded& OnComplete) const
{
	const ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();
			
	TArray<UCharacterDefinitionData*> LoadedAssets;

	for (const FPrimaryAssetId& Id : LoadedAssetsId)
	{
		// 로드가 완료되면 해당 객체를 실제로 가져옵니다.
		UCharacterDefinitionData* LoadedAsset = CastChecked<UCharacterDefinitionData>(LetheAssetManager.GetPrimaryAssetObject(Id));
		LoadedAssets.Emplace(LoadedAsset);
	}

	// 로드된 객체를 콜백으로 전달합니다.
	OnComplete.ExecuteIfBound(LoadedAssets);
}

void UCardDataLoadSubsystem::OnCardViewDataLoaded(const FPrimaryAssetId& SelfViewId, const FPrimaryAssetId& CharacterDefinitionId, const FOnCardViewLoaded& OnComplete) const
{
	const ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();
	
	UCardSelfViewData* SelfViewData = CastChecked<UCardSelfViewData>(LetheAssetManager.GetPrimaryAssetObject(SelfViewId));
	UCharacterDefinitionData* CharacterDefinitionData = CastChecked<UCharacterDefinitionData>(LetheAssetManager.GetPrimaryAssetObject(CharacterDefinitionId));
	
	OnComplete.ExecuteIfBound(SelfViewData, CharacterDefinitionData);
}

void UCardDataLoadSubsystem::ChangeCharacterDecksKeyToSave(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, TMap<uint64, FSavedCharacterDeck>& OutDecks) const
{
	// CharacterTag를 Key로 사용하던 Deck을 세이브하기 위해 CharacterId로 교체합니다.
	for (const auto& Deck : InDecks)
	{
		uint64 CharacterId = 0;
		if (ULetheAssetManager::Get().TryGetCharacterIdByTag(Deck.Key, CharacterId))
		{
			OutDecks.Emplace(CharacterId, Deck.Value);
		}
	}

	// CardTag는 저장되진 않지만, 게임 첫 시작인 경우에 대응하기 위해 여기서도 CardTag를 할당해줍니다.
	for (auto& Deck : OutDecks)
	{
		FillCardTagInSavedCardStruct(Deck.Value);
	}
}

void UCardDataLoadSubsystem::ChangeCharacterDecksKeyToLoad(const TMap<uint64, FSavedCharacterDeck>& InDecks, TMap<FGameplayTag, FSavedCharacterDeck>& OutDecks) const
{
	// CharacterId로 세이브했던 Deck의 Key를 CharacterTag로 교체해 캐싱합니다.
	for (const auto& Deck : InDecks)
	{
		FGameplayTag CharacterTag;
		if (ULetheAssetManager::Get().TryGetCharacterTagById(Deck.Key, CharacterTag))
		{
			OutDecks.Emplace(CharacterTag, Deck.Value);
		}
	}

	// 로드된 카드를 내부에 CardTag를 채워넣습니다.
	for (auto& Deck : OutDecks)
	{
		FillCardTagInSavedCardStruct(Deck.Value);
	}
}

void UCardDataLoadSubsystem::FillCardTagInSavedCardStruct(FSavedCharacterDeck& OutDeck) const
{
	for (FSavedCard& SavedCard : OutDeck.Deck)
	{
		FGameplayTag CardTag;
		if (ULetheAssetManager::Get().TryGetCardTagById(SavedCard.CardId, CardTag))
		{
			SavedCard.CardTag = CardTag;
		}
	}
}
