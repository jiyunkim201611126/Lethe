// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LetheSaveManagerSubsystem.generated.h"

struct FGameplayTag;
class USaveGame;
class UCardViewData;

UCLASS()
class LETHE_API ULetheSaveManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	void SaveDeck() const;
	void LoadDeck();

	bool IsDeckValid();

private:
	UPROPERTY(Config)
	TSubclassOf<USaveGame> DeckSaveGameClass;

	// 카드의 View를 사용할 용도는 아니며, CardTag가 유효한지 검사하기 위해 갖고 있는 DataAsset입니다.
	UPROPERTY(Config)
	TObjectPtr<UCardViewData> CardViewData;
	
	// Key는 캐릭터 태그, Value는 CardTag 10개 배열로 구성된 TMap입니다.
	TMap<FGameplayTag, TArray<FGameplayTag>> CharacterDecks;
	
	// 사용할 수 있는 Card의 Tag들입니다.
	TArray<FGameplayTag> UnlockedCards;
};
