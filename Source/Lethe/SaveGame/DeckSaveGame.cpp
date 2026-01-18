// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckSaveGame.h"

TMap<FGameplayTag, FSavedCharacterDeck> UDeckSaveGame::GetDefaultCharacterDecks()
{
	return DefaultCharacterDecks;
}

TMap<FGameplayTag, FSavedCharacterDeck> UDeckSaveGame::GetDefaultUnlockedCards()
{
	return DefaultUnlockedCards;
}
