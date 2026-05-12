// Copyright JETBLU, Inc. All Rights Reserved.

#include "FloorTransitionAction.h"

#include "Lethe/Manager/World/StageRuntimeSubsystem.h"

void UFloorTransitionAction::Action(const FTriggeredActionContext& ActionContext)
{
	if (UStageRuntimeSubsystem* StageRuntimeSubsystem = GetWorld()->GetSubsystem<UStageRuntimeSubsystem>())
	{
		StageRuntimeSubsystem->StartFloorTransition();
	}
}
