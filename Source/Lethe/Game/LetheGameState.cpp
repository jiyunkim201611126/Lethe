// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

void ALetheGameState::GoDrawPhase()
{
	if (CurrentPlayerTurnState == EPlayerPhaseState::DrawPhase)
	{
		return;
	}
	
	CurrentPlayerTurnState = EPlayerPhaseState::DrawPhase;
	OnChangePlayerTurnStateDelegate.Broadcast(CurrentPlayerTurnState);
}

void ALetheGameState::GoBattlePhase()
{
	if (CurrentPlayerTurnState == EPlayerPhaseState::BattlePhase)
	{
		return;
	}
	
	CurrentPlayerTurnState = EPlayerPhaseState::BattlePhase;
	OnChangePlayerTurnStateDelegate.Broadcast(CurrentPlayerTurnState);
}
