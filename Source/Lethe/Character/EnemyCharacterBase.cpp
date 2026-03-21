// Copyright JETBLU, Inc. All Rights Reserved.

#include "EnemyCharacterBase.h"

#include "Lethe/Controller/AIController/LetheAIController.h"

void AEnemyCharacterBase::SetEnemyAbilityPriority(const int32 InPriority)
{
	AbilityPriority = InPriority;
}

int32 AEnemyCharacterBase::GetEnemyAbilityPriority() const
{
	return AbilityPriority;
}

void AEnemyCharacterBase::ProcessPlanPhase() const
{
	if (const ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->ProcessPlanPhase();
	}
}

void AEnemyCharacterBase::ProcessTelegraphPlan() const
{
	if (const ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->ProcessTelegraphPlan();
	}
}

const FBFSRange& AEnemyCharacterBase::GetAbilityRange() const
{
	return AbilityRange;
}
