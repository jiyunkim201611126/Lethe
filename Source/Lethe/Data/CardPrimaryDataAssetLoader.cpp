// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPrimaryDataAssetLoader.h"

#include "Card/CardDefinitionData.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
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
		// 로드에 실패한 경우에도 콜백을 일단 호출해 완료 이벤트는 발생시켜줍니다.
		OnLoadedCallback.ExecuteIfBound(CharacterTag, {}, bEquipped);
		SelfDestruct();
		return;
	}

	// 필요한 정보들을 캐싱합니다.
	ForCharacterTag = CharacterTag;
	bIsFromEquippedDeck = bEquipped;
	OnAllDataLoaded = OnLoadedCallback;
	LoadRequestedCards = Cards;
	TotalShouldLoadCount = 0;
	CurrentLoadedCount = 0;

	// 로드할 CardTag들을 가져옵니다.
	TArray<FGameplayTag> CardTags;
	for (const FSavedCard& SavedCard : Cards)
	{
		CardTags.Emplace(SavedCard.CardTag);
	}

	const FOnCardDefinitionsLoaded OnDefinitionsLoaded = FOnCardDefinitionsLoaded::CreateUObject(this, &ThisClass::OnCardDefinitionsLoaded);
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
			++TotalShouldLoadCount;
			const FOnCardViewLoaded OnViewLoaded = FOnCardViewLoaded::CreateWeakLambda(this, [this, CardDef](UCardSelfViewData* SelfView, UCharacterDefinitionData* CharacterDefinition)
			{
				OnViewDataLoaded(CardDef, SelfView, CharacterDefinition);
			});
			DataLoadManager->LoadCardViewData(CardDef->CardTag, ForCharacterTag, OnViewLoaded);
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

	int32 RemoveIndex = INDEX_NONE;
	if (CardDefinition)
	{
		for (int32 Index = 0; Index < LoadRequestedCards.Num(); ++Index)
		{
			if (LoadRequestedCards[Index].CardId == CardDefinition->CardId)
			{
				Info.SavedCardInfo = LoadRequestedCards[Index];
				RemoveIndex = Index;
				break;
			}
		}
	}
	if (RemoveIndex != INDEX_NONE)
	{
		LoadRequestedCards.RemoveAt(RemoveIndex, EAllowShrinking::No);
	}
	
	LoadedCardInfos.Emplace(Info);

	++CurrentLoadedCount;
	
	CheckLoadFinished();
}

void UCardPrimaryDataAssetLoader::CheckLoadFinished()
{
	if (CurrentLoadedCount >= TotalShouldLoadCount)
	{
		OnAllDataLoaded.ExecuteIfBound(ForCharacterTag, LoadedCardInfos, bIsFromEquippedDeck);
		SelfDestruct();
	}
}

void UCardPrimaryDataAssetLoader::Destruct()
{
	DataLoadManager = nullptr;
	OnAllDataLoaded.Unbind();
}

void UCardPrimaryDataAssetLoader::SelfDestruct()
{
	if (DataLoadManager)
	{
		DataLoadManager->RemoveLoader(this);
	}
	Destruct();
}
