// Copyright JETBLU, Inc. All Rights Reserved.

#include "FloorTransitionAction.h"

#include "Lethe/Manager/World/BattleStateSaveSubsystem.h"

void UFloorTransitionAction::Action(const FTriggeredActionContext& ActionContext)
{
	if (UBattleStateSaveSubsystem* BattleStateSaveSubsystem = GetWorld()->GetSubsystem<UBattleStateSaveSubsystem>())
	{
	}
}
