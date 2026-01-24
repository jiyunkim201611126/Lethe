// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckSaveGame.h"

const TMap<FGameplayTag, FSavedCharacterDeck>& UDeckSaveGame::GetDefaultEquippedDecks()
{
	return DefaultEquippedDecks;
}

const TMap<FGameplayTag, FSavedCharacterDeck>& UDeckSaveGame::GetDefaultUnequippedDecks()
{
	return DefaultUnequippedDecks;
}
