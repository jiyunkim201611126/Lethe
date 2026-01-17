// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheSaveManagerSubsystem.h"

#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/SaveGame/DeckSaveGame.h"

void ULetheSaveManagerSubsystem::SaveDeck() const
{
	if (UDeckSaveGame* DeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::CreateSaveGameObject(DeckSaveGameClass)))
	{
		DeckSaveGameObject->UnlockedCards = UnlockedCards;
		DeckSaveGameObject->CharacterDecks = CharacterDecks;

		// 여러 개의 세이브 슬롯을 지원할 목적이라면 이 부분을 수정하면 됩니다.
		const FString SlotName = TEXT("DeckSaveSlot");
		UGameplayStatics::SaveGameToSlot(DeckSaveGameObject, SlotName, 0);
	}
}

void ULetheSaveManagerSubsystem::LoadDeck()
{
	const FString SlotName = TEXT("DeckSaveSlot");
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		// 세이브 파일이 존재하는 경우 들어오는 분기입니다.
		if (const UDeckSaveGame* LoadedDeckSaveGameObject = Cast<UDeckSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			UnlockedCards = LoadedDeckSaveGameObject->UnlockedCards;
			CharacterDecks.Reset();
			CharacterDecks = LoadedDeckSaveGameObject->CharacterDecks;
		}
	}
}

bool ULetheSaveManagerSubsystem::IsDeckValid()
{
	for (const auto& CharacterDeck : CharacterDecks)
	{
		if (CharacterDeck.Value.Num() != MAX_DECK_COUNT)
		{
			// 덱에 10장의 카드가 들어있지 않은 경우 false를 반환합니다.
			return false;
		}
		
		for (const FGameplayTag& CardTag : CharacterDeck.Value)
		{
			if (!CardViewData->FindCardInfoByTag(CardTag))
			{
				// CardViewData에 해당 CardTag의 정보가 없는 경우 false를 반환합니다.
				// 게임 업데이트 등으로 인해서 CardTag가 제거됐을 경우를 상정한 로직입니다.
				return false;
			}
		}
	}

	return true;
}
