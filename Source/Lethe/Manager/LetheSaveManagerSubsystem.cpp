// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheSaveManagerSubsystem.h"

#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/SaveGame/DeckSaveGame.h"

void ULetheSaveManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadDeck();
}

void ULetheSaveManagerSubsystem::SaveDeck() const
{
	if (UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass)))
	{
		DeckSaveGameObject->CharacterDecks = CharacterDecks;
		DeckSaveGameObject->UnlockedCards = UnlockedCards;

		// 여러 개의 세이브 슬롯을 지원할 목적이라면 이 부분을 수정하면 됩니다.
		const FString SlotName = TEXT("DeckSaveSlot");
		UGameplayStatics::SaveGameToSlot(DeckSaveGameObject, SlotName, 0);
	}
}

void ULetheSaveManagerSubsystem::LoadDeck()
{
	const FString SlotName = TEXT("DeckSaveSlot");
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		// 세이브 파일이 존재하는 경우 들어오는 분기입니다.
		if (const UDeckSaveGame* LoadedDeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			CharacterDecks.Reset();
			CharacterDecks = LoadedDeckSaveGameObject->CharacterDecks;
			UnlockedCards.Reset();
			UnlockedCards = LoadedDeckSaveGameObject->UnlockedCards;
		}
	}
	else
	{
		// 세이브파일이 존재하지 않는 경우 들어오는 분기입니다.
		if (UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass)))
		{
			CharacterDecks = DeckSaveGameObject->GetDefaultCharacterDecks();
			UnlockedCards = DeckSaveGameObject->GetDefaultUnlockedCards();
			SaveDeck();
		}
	}
}

bool ULetheSaveManagerSubsystem::IsDeckValid()
{
	for (const auto& CharacterDeck : CharacterDecks)
	{
		if (CharacterDeck.Value.Cards.Num() != MAX_DECK_COUNT)
		{
			// 덱에 10장의 카드가 들어있지 않은 경우 false를 반환합니다.
			return false;
		}
	}

	return true;
}

TMap<FGameplayTag, FSavedCharacterDeck> ULetheSaveManagerSubsystem::GetCharacterDecks()
{
	return CharacterDecks;
}

TMap<FGameplayTag, FSavedCharacterDeck> ULetheSaveManagerSubsystem::GetUnlockedCards()
{
	return UnlockedCards;
}
