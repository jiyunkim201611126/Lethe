// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

ALetheGameState::ALetheGameState()
{
	AbilityExecutionManagerComponent = CreateDefaultSubobject<UAbilityExecutionManagerComponent>("AbilityExecutionManagerComponent");
}

void ALetheGameState::GoDrawPhase()
{
	SetPhase(EPhaseState::DrawPhase);
}

void ALetheGameState::GoPlayerTurnPhase()
{
	SetPhase(EPhaseState::PlayerTurnPhase);
}

void ALetheGameState::GoEnemyTurnPhase()
{
	SetPhase(EPhaseState::EnemyTurnPhase);
}

void ALetheGameState::SetPhase(const EPhaseState NewPhase)
{
	const EPhaseState OldPhase = CurrentTurnState;
	if (OldPhase == NewPhase)
	{
		return;
	}
	
	CurrentTurnState = NewPhase;
	
	OnChangeTurnStateDelegate.Broadcast(OldPhase, NewPhase);

	if (NewPhase == EPhaseState::EnemyTurnPhase)
	{
		AbilityExecutionManagerComponent->StartUseAbility();
	}
}

EPhaseState ALetheGameState::GetTurnPhase() const
{
	return CurrentTurnState;
}

void ALetheGameState::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityExecutionManagerComponent->AddEnemyAbilityActivationData(ActivationData);
}

void ALetheGameState::SetTargetTile(const int32 Priority, ATile* TargetTile) const
{
	AbilityExecutionManagerComponent->SetTargetTile(Priority, TargetTile);
}
