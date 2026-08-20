// Copyright JETBLU, Inc. All Rights Reserved.

#include "EnemyCharacterBase.h"

#include "Component/GASManagerComponent.h"
#include "Lethe/Controller/AIController/LetheAIController.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"

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
		return;
	}
	
	if (const ALetheGameState* GameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		GameState->NotifyEnemyPlanResolved();
	}
}

void AEnemyCharacterBase::ProcessCommitPlan() const
{
	if (const ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->ProcessCommitPlan();
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
		AIController->Destroy();
	}

	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->UnregisterEnemy(this);
	}

	// TODO: 애니메이션 재생한 뒤 Destroy되어야 합니다.
	Destroy();
}

void AEnemyCharacterBase::NotifyNoiseHeard() const
{
	if (ALetheAIController* AIController = GetController<ALetheAIController>())
	{
		AIController->StartCombat();
	}
}

void AEnemyCharacterBase::UpdateHiddenByTile_Implementation(const ATile* Tile)
{
	const ALetheAIController* AIController = GetController<ALetheAIController>();
	if (!AIController || !Tile)
	{
		return;
	}

	// 전투 중인 경우 밟은 Tile에 관계 없이 항상 적을 렌더링합니다.
	if (AIController->IsInCombat())
	{
		SetActorHiddenInGame(false);
		return;
	}
	
	switch (Tile->GetTileVisionState())
	{
	case ETileVisionState::Hidden:
	case ETileVisionState::Recognizable:
		if (!IsHidden())
		{
			SetActorHiddenInGame(true);
		}
		break;
	case ETileVisionState::Visible:
		if (IsHidden())
		{
			SetActorHiddenInGame(false);
		}
		break;
	default:
		break;
	}
}

void AEnemyCharacterBase::OnMoveTileChanged(ATile* OldTile, ATile* NewTile)
{
	const ALetheAIController* AIController = GetController<ALetheAIController>();
	if (!AIController || !OldTile || !NewTile)
	{
		return;
	}
	
	// 전투 중이 아니라면 시야를 갱신할 필요가 없습니다.
	if (!AIController->IsInCombat())
	{
		return;
	}
	
	if (URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>())
	{
		RoomManagerSubsystem->NotifyCharacterTileChanged(this, OldTile, NewTile);
	}
}
