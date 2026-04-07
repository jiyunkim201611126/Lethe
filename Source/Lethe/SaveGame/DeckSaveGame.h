// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "SavedCardTypes.h"
#include "GameFramework/SaveGame.h"
#include "DeckSaveGame.generated.h"

class ULetheGameplayAbility;

UCLASS()
class LETHE_API UDeckSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	TMap<FGameplayTag, FSavedCharacterDeck> GetDefaultEquippedDecks();
	TMap<FGameplayTag, FSavedCharacterDeck> GetDefaultUnequippedDecks();

public:
	/** Key는 CharacterId, Value는 CardTag 10개 배열로 구성된 TMap입니다. */
	UPROPERTY()
	TMap<uint64, FSavedCharacterDeck> EquippedDecks;

	/** 장착하지 않은 상태의 사용할 수 있는 CardTag들입니다. */
	UPROPERTY()
	TMap<uint64, FSavedCharacterDeck> UnequippedDecks;

protected:
	/** 게임을 처음 시작하거나, 캐릭터를 처음 언락한 경우 장착되어 있을 카드를 여기서 설정할 수 있습니다. */
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FSavedCharacterDeck> DefaultEquippedDecks;
	
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FSavedCharacterDeck> DefaultUnequippedDecks;
};
