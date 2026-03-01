// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "Components/StateTreeAIComponent.h"
#include "Lethe/Game/LetheGameState.h"

ALetheAIController::ALetheAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}

void ALetheAIController::BeginPlay()
{
	Super::BeginPlay();

	if (ALetheGameState* LetheGameState = Cast<ALetheGameState>(GetWorld()->GetGameState()))
	{
		LetheGameState->OnChangeTurnStateDelegate.AddUObject(this, &ThisClass::OnPhaseStateChanged);
	}
}

void ALetheAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = Cast<ALetheGameState>(GetWorld()->GetGameState()))
	{
		LetheGameState->OnChangeTurnStateDelegate.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StartLogic();
	}
}

void ALetheAIController::OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase)
{
	if (!StateTreeAIComponent)
	{
		return;
	}
	
	if (OldPhase == EPhaseState::EnemyTurnPhase)
	{
		StateTreeAIComponent->StopLogic(FString(TEXT("EnemyTurnEnded")));
	}

	if (NewPhase == EPhaseState::EnemyTurnPhase)
	{
		StateTreeAIComponent->StartLogic();
	}
}
