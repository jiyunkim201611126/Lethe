// Copyright JETBLU, Inc. All Rights Reserved.

#include "EnemyCharacterBase.h"

#include "Lethe/Controller/AIController/LetheAIController.h"
#include "Lethe/Game/GameState/LetheGameState.h"

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

void AEnemyCharacterBase::OnDamageTaken()
{
	Super::OnDamageTaken();
	
	if (ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->StartCombat();
	}
}

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

	// TODO: 애니메이션 재생한 뒤 Destroy되어야 합니다.
	Destroy();
}

void AEnemyCharacterBase::StartCombat() const
{
	if (ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->StartCombat();
	}
}

const FBFSRange& AEnemyCharacterBase::GetAbilityRange() const
{
	return AbilityRange;
}
