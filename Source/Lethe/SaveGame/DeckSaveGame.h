// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameFramework/SaveGame.h"
#include "Lethe/Lethe.h"
#include "DeckSaveGame.generated.h"

class ULetheGameplayAbility;

USTRUCT(BlueprintType)
struct FSavedCard
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	uint64 CardId;

	// Id로 찾은 Tag가 런타임 중 동적으로 채워집니다.
	// Id는 출시 이후 절대 변경되지 않으나, CardTag는 필요에 따라 변경될 수 있기 때문에 이와 같은 방법을 사용합니다.
	FGameplayTag CardTag;

	UPROPERTY()
	int32 CardLevel = 1;
};

/**
 * SaveGame에 사용하기 위해선 UPROPERTY를 반드시 붙여야 하나, TArray는 UPROPERTY로 선언된 TMap 내부 자료형으로 사용할 수 없습니다.
 * 이를 우회하기 위해 선언된 구조체입니다.
 */
USTRUCT(BlueprintType)
struct FSavedCharacterDeck
{
	GENERATED_BODY()

	FSavedCharacterDeck()
	{
		Deck.Reserve(MAX_DECK_COUNT);
	}

	UPROPERTY(EditDefaultsOnly)
	TArray<FSavedCard> Deck;
};

UCLASS()
class LETHE_API UDeckSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	TMap<FGameplayTag, FSavedCharacterDeck> GetDefaultEquippedDecks();
	TMap<FGameplayTag, FSavedCharacterDeck> GetDefaultUnequippedDecks();

public:
	// Key는 CharacterId, Value는 CardTag 10개 배열로 구성된 TMap입니다.
	UPROPERTY()
	TMap<uint64, FSavedCharacterDeck> EquippedDecks;

	// 장착하지 않은 상태의 사용할 수 있는 CardTag들입니다.
	UPROPERTY()
	TMap<uint64, FSavedCharacterDeck> UnequippedDecks;

protected:
	// 게임을 처음 시작하거나, 캐릭터를 처음 언락한 경우 장착되어 있을 카드를 여기서 설정할 수 있습니다.
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FSavedCharacterDeck> DefaultEquippedDecks;
	
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FSavedCharacterDeck> DefaultUnequippedDecks;
};
