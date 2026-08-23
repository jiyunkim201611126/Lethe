// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "TurnPhaseState.generated.h"

UENUM()
enum class ETurnPhaseState : uint8
{
	None,
	
	/** 전투와 관계 없는 페이즈 */
	EnemyPlanPhase,

	/** 비전투 페이즈 */
	PlayerMovePhase,

	/** 전투 페이즈 */
	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangeTurnPhaseState, const ETurnPhaseState /* OldTurnPhaseState */, const ETurnPhaseState /* NewTurnPhaseState */);
