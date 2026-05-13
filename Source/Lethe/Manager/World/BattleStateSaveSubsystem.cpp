// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleStateSaveSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/Character/PlayerCharacterBase.h"

void UBattleStateSaveSubsystem::SaveBattleState(const FBattleStateSaveContext& Context)
{
	UBattleStateSaveGame* BattleStateSaveGameObject = CastChecked<UBattleStateSaveGame>(UGameplayStatics::CreateSaveGameObject(BattleStateSaveGameClass));

	for (const APlayerCharacterBase* PlayerCharacter : Context.PlayerCharacters)
	{
		if (PlayerCharacter)
		{
			const int64 CharacterId = PlayerCharacter->GetCharacterId();
			const int64 PlayerOrderIndex = PlayerCharacter->GetPlayerOrderIndex();
			BattleStateSaveGameObject->SavePlayerCharacterAttributes(CharacterId, PlayerOrderIndex, PlayerCharacter->GetAbilitySystemComponent());
		}
	}

	UGameplayStatics::SaveGameToSlot(BattleStateSaveGameObject, SlotName, 0);

	// 세이브를 완료했으므로, 일관된 작동 보장을 위해 한 번 로드합니다.
	LoadBattleState(Context);
}

void UBattleStateSaveSubsystem::LoadBattleState(const FBattleStateSaveContext& Context)
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		// 세이브 파일이 존재하는 경우 들어오는 분기입니다.
		const UBattleStateSaveGame* LoadedBattleStateSaveGameObject = CastChecked<UBattleStateSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		
		for (const APlayerCharacterBase* PlayerCharacter : Context.PlayerCharacters)
		{
			if (PlayerCharacter)
			{
				LoadedBattleStateSaveGameObject->ApplyPlayerCharacterAttributes(PlayerCharacter->GetCharacterId(), PlayerCharacter->GetAbilitySystemComponent());
			}
		}
	}
	else
	{
		// 세이브파일이 존재하지 않는 경우 들어오는 분기입니다.
		SaveBattleState(Context);
	}
}
