// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameplayTagContainer.h"
#include "Lethe/Lethe.h"
#include "DeckSaveGame.generated.h"

class ULetheGameplayAbility;

USTRUCT(BlueprintType)
struct FSavedCard
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
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
		Cards.Reserve(MAX_DECK_COUNT);
	}

	UPROPERTY(EditDefaultsOnly)
	TArray<FSavedCard> Cards;
};

UCLASS()
class LETHE_API UDeckSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	TMap<FGameplayTag, FSavedCharacterDeck> GetDefaultCharacterDecks();
	TMap<FGameplayTag, FSavedCharacterDeck> GetDefaultUnlockedCards();

public:
	// Key는 캐릭터 태그, Value는 CardTag 10개 배열로 구성된 TMap입니다.
	UPROPERTY()
	TMap<FGameplayTag, FSavedCharacterDeck> CharacterDecks;

	// 장착하지 않은 상태의 사용할 수 있는 CardTag들입니다.
	UPROPERTY()
	TMap<FGameplayTag, FSavedCharacterDeck> UnlockedCards;

protected:
	// 게임을 처음 시작하거나, 캐릭터를 처음 언락한 경우 장착되어 있을 카드를 여기서 설정할 수 있습니다.
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FSavedCharacterDeck> DefaultCharacterDecks;
	
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, FSavedCharacterDeck> DefaultUnlockedCards;
};
