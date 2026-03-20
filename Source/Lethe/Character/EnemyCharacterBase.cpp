// Copyright JETBLU, Inc. All Rights Reserved.

#include "EnemyCharacterBase.h"

#include "Lethe/Controller/AIController/LetheAIController.h"

void AEnemyCharacterBase::SetEnemyAbilityPriority(const int32 InPriority) const
{
	if (ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->SetAbilityPriority(InPriority);
	}
}

const FBFSRange& AEnemyCharacterBase::GetAbilityRange() const
{
	return AbilityRange;
}
