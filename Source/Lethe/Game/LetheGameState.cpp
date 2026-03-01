// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

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
}

EPhaseState ALetheGameState::GetTurnPhase() const
{
	return CurrentTurnState;
}
