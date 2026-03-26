// Copyright JETBLU, Inc. All Rights Reserved.

#include "EnemyCharacterBase.h"

#include "Lethe/Controller/AIController/LetheAIController.h"
#include "Lethe/Game/GameState/LetheGameState.h"

void AEnemyCharacterBase::Die()
{
	Super::Die();
	
	if (ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->DeactivateArrow();
		AIController->UnPossess();
	}

	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->UnregisterEnemy(this);
	}

	// TODO: 보통은 애니메이션 재생한 뒤 Destroy되는 게 맞습니다.
	Destroy();
}

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
