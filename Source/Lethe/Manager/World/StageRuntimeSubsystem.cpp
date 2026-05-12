// Copyright JETBLU, Inc. All Rights Reserved.

#include "StageRuntimeSubsystem.h"

#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Game/GameMode/BattleGameMode.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void UStageRuntimeSubsystem::RegisterFloorActor(AActor* InActor)
{
	FloorActors.Add(InActor);
}

void UStageRuntimeSubsystem::StartFloorTransition()
{
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	const ABattleGameMode* BattleGameMode = GetWorld()->GetAuthGameMode<ABattleGameMode>();
	if (!TileManagerSubsystem || !RoomManagerSubsystem || !BattleGameMode)
	{
		return;
	}

	AController* Controller = BattleGameMode->GetController();
	ALethePlayerController* PlayerController = Cast<ALethePlayerController>(Controller);
	if (!PlayerController)
	{
		return;
	}

	PlayerController->SetIsFloorTransitioning(true);

	for (const auto& Actor : FloorActors)
	{
		if (Actor.IsValid())
		{
			Actor->Destroy();
		}
	}
	FloorActors.Empty();
	
	TileManagerSubsystem->Clear();
	RoomManagerSubsystem->Clear();
	
	PlayerController->ResetForFloorTransition();

	BattleGameMode->OnFloorTransitionStarted();

	PlayerController->SetIsFloorTransitioning(false);
}
