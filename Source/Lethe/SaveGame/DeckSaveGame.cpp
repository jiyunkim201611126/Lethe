// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckSaveGame.h"

TMap<FGameplayTag, FSavedCharacterDeck> UDeckSaveGame::GetDefaultEquippedDecks()
{
	return DefaultEquippedDecks;
}

TMap<FGameplayTag, FSavedCharacterDeck> UDeckSaveGame::GetDefaultUnequippedDecks()
{
	return DefaultUnequippedDecks;
}
