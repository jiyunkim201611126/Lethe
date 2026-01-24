// Copyright JETBLU, Inc. All Rights Reserved.

#include "DataAssetLoader.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/SaveGame/DeckSaveGame.h"
#include "Engine/GameInstance.h"

UDataAssetLoader* UDataAssetLoader::CreateLoader(UObject* Outer)
{
	UDataAssetLoader* Loader = NewObject<UDataAssetLoader>(Outer);

	if (Loader->GetWorld())
	{
		Loader->DataLoadManager = Loader->GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>();
	}

	return Loader;
}

void UDataAssetLoader::LoadCardData(const FGameplayTag& CharacterTag, const TArray<FSavedCard>& Cards, const bool bEquipped, const FOnAllCardDataLoaded& OnLoadedCallback)
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
	CardTags.Reserve(Cards.Num());
	for (const FSavedCard& SavedCard : Cards)
	{
		CardTags.Emplace(SavedCard.CardTag);
	}

	const FOnCardDefinitionsLoaded OnDefinitionsLoaded = FOnCardDefinitionsLoaded::CreateUObject(this, &UDataAssetLoader::OnCardDefinitionsLoaded);
	DataLoadManager->LoadCardDefinitionData(CardTags, OnDefinitionsLoaded);
}

void UDataAssetLoader::OnCardDefinitionsLoaded(const TArray<UCardDefinitionData*>& LoadedDefinitions)
{
	if (LoadedDefinitions.IsEmpty())
	{
		CheckIfFinished();
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
	
	CheckIfFinished();
}

void UDataAssetLoader::OnViewDataLoaded(UCardDefinitionData* CardDefinition, UCardSelfViewData* SelfView, UCharacterDefinitionData* CharacterDefinition)
{
	FLoadedCardInfo Info;
	Info.CardDefinition = CardDefinition;
	Info.SelfViewData = SelfView;
	Info.CharacterDefinition = CharacterDefinition;
	LoadedCardInfos.Emplace(Info);

	CurrentLoadCount++;
	
	CheckIfFinished();
}

void UDataAssetLoader::CheckIfFinished()
{
	if (CurrentLoadCount >= TotalLoadCount)
	{
		OnAllDataLoaded.ExecuteIfBound(ForCharacterTag, LoadedCardInfos, bIsFromEquippedDeck);
		SelfDestruct();
	}
}

void UDataAssetLoader::SelfDestruct()
{
	if (IsValid(this))
	{
		ConditionalBeginDestroy();
	}
}
