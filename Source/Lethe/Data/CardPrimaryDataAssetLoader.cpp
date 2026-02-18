// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPrimaryDataAssetLoader.h"

#include "Card/CardDefinitionData.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
#include "Lethe/SaveGame/DeckSaveGame.h"
#include "Engine/GameInstance.h"

UCardPrimaryDataAssetLoader* UCardPrimaryDataAssetLoader::CreateLoader(UObject* Outer)
{
	UCardPrimaryDataAssetLoader* Loader = NewObject<UCardPrimaryDataAssetLoader>(Outer);

	if (Loader->GetWorld())
	{
		Loader->DataLoadManager = Loader->GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>();
	}

	return Loader;
}

void UCardPrimaryDataAssetLoader::LoadCardData(const FGameplayTag& CharacterTag, const TArray<FSavedCard>& Cards, const bool bEquipped, const FOnAllCardDataLoaded& OnLoadedCallback)
{
	if (!DataLoadManager || Cards.IsEmpty())
	{
		SelfDestruct();
		return;
	}

	// 필요한 정보들을 캐싱합니다.
	ForCharacterTag = CharacterTag;
	bIsFromEquippedDeck = bEquipped;
	OnAllDataLoaded = OnLoadedCallback;
	TotalLoadCount = Cards.Num();
	CurrentLoadCount = 0;
	LoadedCardInfos.Reserve(TotalLoadCount);

	// 로드할 CardTag들을 가져옵니다.
	TArray<FGameplayTag> CardTags;
	CardTags.Reserve(TotalLoadCount);
	for (const FSavedCard& SavedCard : Cards)
	{
		CardTags.Emplace(SavedCard.CardTag);
	}

	const FOnCardDefinitionsLoaded OnDefinitionsLoaded = FOnCardDefinitionsLoaded::CreateUObject(this, &UCardPrimaryDataAssetLoader::OnCardDefinitionsLoaded);
	DataLoadManager->LoadCardDefinitionData(CardTags, OnDefinitionsLoaded);
}

void UCardPrimaryDataAssetLoader::OnCardDefinitionsLoaded(const TArray<UCardDefinitionData*>& LoadedDefinitions)
{
	if (LoadedDefinitions.IsEmpty())
	{
		CheckLoadFinished();
		return;
	}

	for (UCardDefinitionData* CardDef : LoadedDefinitions)
	{
		if (CardDef)
		{
			const FOnCardViewLoaded OnViewLoaded = FOnCardViewLoaded::CreateWeakLambda(this, [this, CardDef](UCardSelfViewData* SelfView, UCharacterDefinitionData* CharacterDefinition)
			{
				OnViewDataLoaded(CardDef, SelfView, CharacterDefinition);
			});
			
			DataLoadManager->LoadCardViewData(CardDef->CardTag, ForCharacterTag, OnViewLoaded);
		}
		else
		{
			TotalLoadCount--;
		}
	}
	
	CheckLoadFinished();
}

void UCardPrimaryDataAssetLoader::OnViewDataLoaded(UCardDefinitionData* CardDefinition, UCardSelfViewData* SelfView, UCharacterDefinitionData* CharacterDefinition)
{
	FLoadedCardInfo Info;
	Info.CardDefinition = CardDefinition;
	Info.SelfViewData = SelfView;
	Info.CharacterDefinition = CharacterDefinition;
	LoadedCardInfos.Emplace(Info);

	CurrentLoadCount++;
	
	CheckLoadFinished();
}

void UCardPrimaryDataAssetLoader::CheckLoadFinished()
{
	if (CurrentLoadCount >= TotalLoadCount)
	{
		OnAllDataLoaded.ExecuteIfBound(ForCharacterTag, LoadedCardInfos, bIsFromEquippedDeck);
		SelfDestruct();
	}
}

void UCardPrimaryDataAssetLoader::SelfDestruct()
{
	if (IsValid(this))
	{
		DataLoadManager = nullptr;
		OnAllDataLoaded = nullptr;
		
		ConditionalBeginDestroy();
	}
}
