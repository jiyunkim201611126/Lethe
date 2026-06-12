// Copyright JETBLU, Inc. All Rights Reserved.

#include "CombatInterface.h"

bool ICombatInterface::IsDead()
{
	return true;
}

ETeamSide ICombatInterface::GetTeamSide() const
{
	return ETeamSide::None;
}
