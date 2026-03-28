// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckManagerSubsystem.h"

#include "DataLoadManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UDeckManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EquippedDecks.Reserve(PLAYER_CHARACTER_NUMBER);
	UnequippedDecks.Reserve(PLAYER_CHARACTER_NUMBER);
	LoadDeck();
}

void UDeckManagerSubsystem::SaveDeck(const TMap<FGameplayTag, FSavedCharacterDeck>& InEquippedDecks, const TMap<FGameplayTag, FSavedCharacterDeck>& InUnequippedDecks)
{
	UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass));
	const UDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>();
	if (DeckSaveGameObject && CardDataLoadManagerSubsystem)
	{
		// Tag 기준으로 캐싱해두었던 데이터를 Id 기준으로 변경해 세이브합니다.
		TMap<uint64, FSavedCharacterDeck> OutEquippedDecks;
		CardDataLoadManagerSubsystem->ChangeCharacterDecksKeyToSave(InEquippedDecks, OutEquippedDecks);

		TMap<uint64, FSavedCharacterDeck> OutUnequippedDecks;
		CardDataLoadManagerSubsystem->ChangeCharacterDecksKeyToSave(InUnequippedDecks, OutUnequippedDecks);
		
		DeckSaveGameObject->EquippedDecks = OutEquippedDecks;
		DeckSaveGameObject->UnequippedDecks = OutUnequippedDecks;

		// 여러 개의 세이브 슬롯을 지원할 목적이라면 이 부분을 수정하면 됩니다.
		UGameplayStatics::SaveGameToSlot(DeckSaveGameObject, SlotName, 0);

		// 세이브를 완료했으므로, 일관된 작동 보장을 위해 한 번 로드합니다.
		LoadDeck();
	}
}

void UDeckManagerSubsystem::LoadDeck()
{
	if (const UDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>())
	{
		if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
		{
			// 세이브 파일이 존재하는 경우 들어오는 분기입니다.
			const UDeckSaveGame* LoadedDeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
			if (LoadedDeckSaveGameObject)
			{
				// Id 기준으로 세이브되었던 데이터를 Tag 기준으로 변경해 로드합니다.
				TMap<FGameplayTag, FSavedCharacterDeck> OutEquippedDecks;
				CardDataLoadManagerSubsystem->ChangeCharacterDecksKeyToLoad(LoadedDeckSaveGameObject->EquippedDecks, OutEquippedDecks);
			
				EquippedDecks.Reset();
				EquippedDecks = OutEquippedDecks;
			
				TMap<FGameplayTag, FSavedCharacterDeck> OutUnequippedDecks;
				CardDataLoadManagerSubsystem->ChangeCharacterDecksKeyToLoad(LoadedDeckSaveGameObject->UnequippedDecks, OutUnequippedDecks);
			
				UnequippedDecks.Reset();
				UnequippedDecks = OutUnequippedDecks;
			}
		}
		else
		{
			// 세이브파일이 존재하지 않는 경우 들어오는 분기입니다.
			if (UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass)))
			{
				SaveDeck(DeckSaveGameObject->GetDefaultEquippedDecks(), DeckSaveGameObject->GetDefaultUnequippedDecks());
			}
		}
	}
}

bool UDeckManagerSubsystem::IsDeckValid()
{
	for (const auto& CharacterDeck : EquippedDecks)
	{
		if (CharacterDeck.Value.Deck.Num() != MAX_DECK_COUNT)
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
