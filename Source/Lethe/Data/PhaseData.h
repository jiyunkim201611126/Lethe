// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhaseData.generated.h"

UENUM()
enum class EPhaseState : uint8
{
	None,
	EnemyPlanningPhase,

	/** 비전투 페이즈 */
	PlayerMovePhase,

	/** 전투 페이즈 */
	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};
