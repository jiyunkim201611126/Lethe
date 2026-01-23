// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckManagerSubsystem.h"

#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/SaveGame/DeckSaveGame.h"

void UDeckManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	LoadDeck();
}

void UDeckManagerSubsystem::SaveDeck() const
{
	if (UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass)))
	{
		DeckSaveGameObject->EquippedDecks = EquippedDecks;
		DeckSaveGameObject->UnequippedDecks = UnequippedDecks;

		// 여러 개의 세이브 슬롯을 지원할 목적이라면 이 부분을 수정하면 됩니다.
		const FString SlotName = TEXT("DeckSaveSlot");
		UGameplayStatics::SaveGameToSlot(DeckSaveGameObject, SlotName, 0);
	}
}

void UDeckManagerSubsystem::LoadDeck()
{
	const FString SlotName = TEXT("DeckSaveSlot");
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		// 세이브 파일이 존재하는 경우 들어오는 분기입니다.
		if (const UDeckSaveGame* LoadedDeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			EquippedDecks.Reset();
			EquippedDecks = LoadedDeckSaveGameObject->EquippedDecks;
			UnequippedDecks.Reset();
			UnequippedDecks = LoadedDeckSaveGameObject->UnequippedDecks;
		}
	}
	else
	{
		// 세이브파일이 존재하지 않는 경우 들어오는 분기입니다.
		if (UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass)))
		{
			EquippedDecks = DeckSaveGameObject->GetDefaultEquippedDecks();
			UnequippedDecks = DeckSaveGameObject->GetDefaultUnequippedDecks();
			SaveDeck();
		}
	}
}

bool UDeckManagerSubsystem::IsDeckValid()
{
	for (const auto& CharacterDeck : EquippedDecks)
	{
		if (CharacterDeck.Value.Cards.Num() != MAX_DECK_COUNT)
		{
			// 덱에 10장의 카드가 들어있지 않은 경우 false를 반환합니다.
			return false;
		}
	}

	return true;
}

TMap<FGameplayTag, FSavedCharacterDeck> UDeckManagerSubsystem::GetEquippedDecks()
{
	return EquippedDecks;
}

TMap<FGameplayTag, FSavedCharacterDeck> UDeckManagerSubsystem::GetUnequippedDecks()
{
	return UnequippedDecks;
}
