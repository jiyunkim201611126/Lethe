// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/SaveGame/DeckSaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "DeckManagerSubsystem.generated.h"

/**
 * USaveGame을 통한 덱 세이브 로드를 담당하는 Subsystem입니다.
 */
UCLASS(Config = Game)
class LETHE_API UDeckManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	void SaveDeck(const TMap<FGameplayTag, FSavedCharacterDeck>& InEquippedDecks, const TMap<FGameplayTag, FSavedCharacterDeck>& InUnequippedDecks);
	void LoadDeck();

	bool IsDeckValid();

	TMap<FGameplayTag, FSavedCharacterDeck> GetEquippedDecks();
	TMap<FGameplayTag, FSavedCharacterDeck> GetUnequippedDecks(); 

private:
	UPROPERTY(Config)
	TSubclassOf<UDeckSaveGame> DeckSaveGameClass;

	const FString SlotName = TEXT("DeckSaveSlot");
	
	// Key는 캐릭터 태그, Value는 CardTag 10개 배열로 구성된 TMap입니다.
	TMap<FGameplayTag, FSavedCharacterDeck> EquippedDecks;
	
	// Key와 Value의 구성은 위와 같습니다. 장착하지 않은 상태의 사용할 수 있는 CardTag들입니다.
	TMap<FGameplayTag, FSavedCharacterDeck> UnequippedDecks;
};
