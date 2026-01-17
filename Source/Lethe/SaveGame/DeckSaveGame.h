// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "DeckSaveGame.generated.h"

struct FGameplayTag;

UCLASS()
class LETHE_API UDeckSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UDeckSaveGame();

	// Key는 캐릭터 태그, Value는 CardTag 10개 배열로 구성된 TMap입니다.
	UPROPERTY()
	TMap<FGameplayTag, TArray<FGameplayTag>> CharacterDecks;

	// 사용할 수 있는 Card의 Tag들입니다.
	UPROPERTY()
	TArray<FGameplayTag> UnlockedCards;
};
