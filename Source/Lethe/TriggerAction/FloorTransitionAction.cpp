// Copyright JETBLU, Inc. All Rights Reserved.

#include "FloorTransitionAction.h"

#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"
#include "Lethe/Manager/World/BattleStateSaveSubsystem.h"
#include "Lethe/Manager/World/LevelManagerSubsystem.h"

void UFloorTransitionAction::Action(const FTriggeredActionContext& ActionContext)
{
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	const UBattleStateSaveSubsystem* BattleStateSaveSubsystem = GetWorld()->GetSubsystem<UBattleStateSaveSubsystem>();
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	ULevelManagerSubsystem* LevelManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>();

	if (!LetheGameState || !BattleStateSaveSubsystem || !TileManagerSubsystem || !RoomManagerSubsystem || !LevelManagerSubsystem)
	{
		return;
	}

	FBattleStateSaveContext Context;
	for (AActor* PlayerCharacterActor : LetheGameState->GetPlayerCharacters())
	{
		if (APlayerCharacterBase* PlayerCharacter = Cast<APlayerCharacterBase>(PlayerCharacterActor))
		{
			Context.PlayerCharacters.Add(PlayerCharacter);
		}
	}
	BattleStateSaveSubsystem->SaveBattleState(Context);

	TileManagerSubsystem->Clear();
	RoomManagerSubsystem->Clear();

	LevelManagerSubsystem->StartLevelTransition(LevelManagerSubsystem->GetCurrentLevelType(), FString("FloorTransition"));
}
