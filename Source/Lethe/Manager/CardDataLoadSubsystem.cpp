// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardDataLoadSubsystem.h"

#include "Engine/DataAsset.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Manager/EngineSystem/LetheAssetManager.h"
#include "Lethe/SaveGame/SavedCardTypes.h"

void UCardDataLoadSubsystem::Deinitialize()
{
	PendingCardDataLoadRequests.Empty();
	
	Super::Deinitialize();
}

void UCardDataLoadSubsystem::LoadCardData(const FGameplayTag& CharacterTag, const TArray<FSavedCard>& Cards, const bool bEquipped, const FOnAllCardDataLoaded& OnLoadedCallback)
{
	if (Cards.IsEmpty())
	{
		// 로드에 실패한 경우에도 콜백을 일단 호출해 완료 이벤트는 발생시켜줍니다.
		OnLoadedCallback.ExecuteIfBound(CharacterTag, {}, bEquipped);
		return;
	}

	// 요청 정보를 묶어 캐싱합니다.
	const uint64 RequestId = NextCardDataLoadRequestId++;
	FPendingCardDataLoadRequest& Request = PendingCardDataLoadRequests.Emplace(RequestId);
	Request.CharacterTag = CharacterTag;
	Request.LoadRequestedCards = Cards;
	Request.bEquipped = bEquipped;
	Request.OnLoadedCallback = OnLoadedCallback;

	// 로드할 CardTag들을 가져옵니다.
	TArray<FGameplayTag> CardTags;
	CardTags.Reserve(Cards.Num());
	for (const FSavedCard& SavedCard : Cards)
	{
		CardTags.Emplace(SavedCard.CardTag);
	}

	const FOnPrimaryDataAssetsLoaded OnDefinitionsLoaded = FOnPrimaryDataAssetsLoaded::CreateWeakLambda(this, [this, RequestId](const TArray<UPrimaryDataAsset*>& LoadedDefinitions)
	{
		OnCardDefinitionsLoadedForRequest(RequestId, LoadedDefinitions);
	});
	ULetheAssetManager::Get().LoadPrimaryDataAssets(CardTags, OnDefinitionsLoaded);

	const FOnPrimaryDataAssetsLoaded OnCharacterDefinitionsLoaded = FOnPrimaryDataAssetsLoaded::CreateWeakLambda(this, [this, RequestId](const TArray<UPrimaryDataAsset*>& LoadedCharacterDefinitions)
	{
		OnCharacterDefinitionsLoadedForRequest(RequestId, LoadedCharacterDefinitions);
	});
	ULetheAssetManager::Get().LoadPrimaryDataAssets({ CharacterTag }, OnCharacterDefinitionsLoaded);
}

void UCardDataLoadSubsystem::OnCardDefinitionsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedDefinitions)
{
	FPendingCardDataLoadRequest* Request = PendingCardDataLoadRequests.Find(RequestId);
	if (!Request)
	{
		return;
	}

	Request->LoadedCardDefinitions.Reset(LoadedDefinitions.Num());
	for (UPrimaryDataAsset* LoadedDefinition : LoadedDefinitions)
	{
		Request->LoadedCardDefinitions.Emplace(CastChecked<UCardDefinitionData>(LoadedDefinition));
	}
	Request->bCardDefinitionsLoaded = true;

	TryFinishCardDataLoad(RequestId);
}

void UCardDataLoadSubsystem::OnCharacterDefinitionsLoadedForRequest(const uint64 RequestId, const TArray<UPrimaryDataAsset*>& LoadedCharacterDefinitions)
{
	FPendingCardDataLoadRequest* Request = PendingCardDataLoadRequests.Find(RequestId);
	if (!Request)
	{
		return;
	}

	Request->LoadedCharacterDefinitions.Reset(LoadedCharacterDefinitions.Num());
	for (UPrimaryDataAsset* LoadedCharacterDefinition : LoadedCharacterDefinitions)
	{
		Request->LoadedCharacterDefinitions.Emplace(CastChecked<UCharacterDefinitionData>(LoadedCharacterDefinition));
	}
	Request->bCharacterDefinitionLoaded = true;

	TryFinishCardDataLoad(RequestId);
}

void UCardDataLoadSubsystem::TryFinishCardDataLoad(const uint64 RequestId)
{
	FPendingCardDataLoadRequest* Request = PendingCardDataLoadRequests.Find(RequestId);
	if (!Request || !Request->bCardDefinitionsLoaded || !Request->bCharacterDefinitionLoaded)
	{
		return;
	}

	UCharacterDefinitionData* CharacterDefinition = Request->LoadedCharacterDefinitions.IsEmpty() ? nullptr : Request->LoadedCharacterDefinitions[0].Get();

	TArray<FLoadedCardInfo> LoadedCardInfos;
	for (const FSavedCard& SavedCard : Request->LoadRequestedCards)
	{
		FLoadedCardInfo Info;
		Info.SavedCardInfo = SavedCard;
		Info.CharacterDefinition = CharacterDefinition;

		for (const TObjectPtr<UCardDefinitionData>& CardDefinition : Request->LoadedCardDefinitions)
		{
			if (CardDefinition && CardDefinition->CardId == SavedCard.CardId)
			{
				Info.CardDefinition = CardDefinition;
				break;
			}
		}

		LoadedCardInfos.Emplace(Info);
	}

	const FGameplayTag CharacterTag = Request->CharacterTag;
	const bool bEquipped = Request->bEquipped;
	const FOnAllCardDataLoaded OnLoadedCallback = Request->OnLoadedCallback;
	
	PendingCardDataLoadRequests.Remove(RequestId);

	OnLoadedCallback.ExecuteIfBound(CharacterTag, LoadedCardInfos, bEquipped);
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
