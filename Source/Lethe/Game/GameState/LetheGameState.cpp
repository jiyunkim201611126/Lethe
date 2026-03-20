// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

#include "Lethe/Controller/AIController/LetheAIController.h"

ALetheGameState::ALetheGameState()
{
	AbilityResolverComponent = CreateDefaultSubobject<UAbilityResolverComponent>("AbilityResolverComponent");
}

void ALetheGameState::BeginPlay()
{
	Super::BeginPlay();
	
	AbilityResolverComponent->OnEnemyAbilityActivated.AddUObject(this, &ThisClass::OnEnemyAbilityActivated);
	AbilityResolverComponent->OnAllEnemyAbilityResolved.BindUObject(this, &ThisClass::OnAllEnemyAbilityResolved);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->OnEnemyAbilityActivated.RemoveAll(this);
	AbilityResolverComponent->OnAllEnemyAbilityResolved.Unbind();
	
	Super::EndPlay(EndPlayReason);
}

void ALetheGameState::GoEnemyMovePhase()
{
	SetPhase(EPhaseState::EnemyMovePhase);
}

void ALetheGameState::GoPlayerPhase()
{
	// 전투에 참여 중인 적들을 확인합니다.
	RebuildCurrentInBattleEnemies();
	
	if (CurrentInBattleEnemies.IsEmpty())
	{
		SetPhase(EPhaseState::PlayerMovePhase);
	}
	else
	{
		SetPhase(EPhaseState::DrawPhase);
	}
}

void ALetheGameState::GoPlayerTurnPhase()
{
	SetPhase(EPhaseState::PlayerTurnPhase);
}

void ALetheGameState::RequestTurnEnd()
{
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		SetPhase(EPhaseState::EnemyMovePhase);
	}
	else if (CurrentPhaseState == EPhaseState::PlayerTurnPhase)
	{
		SetPhase(EPhaseState::EnemyTurnPhase);
	}
}

void ALetheGameState::RegisterEnemy(AActor* Enemy)
{
	RegisteredEnemies.Emplace(Enemy);
	PendingMoveEnemies.Emplace(Enemy);
}

void ALetheGameState::RemovePendingEnemyMove(AActor* Enemy)
{
	PendingMoveEnemies.Remove(Enemy);

	if (PendingMoveEnemies.IsEmpty())
	{
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

void ALetheGameState::OnAllEnemyAbilityResolved()
{
	if (CurrentPhaseState == EPhaseState::EnemyMovePhase)
	{
		GoPlayerPhase();
	}
	else if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		GoEnemyMovePhase();
	}
}

void ALetheGameState::SetPhase(const EPhaseState NewPhase)
{
	const EPhaseState OldPhase = CurrentPhaseState;
	if (OldPhase == NewPhase)
	{
		return;
	}
	
	CurrentPhaseState = NewPhase;

	if (CurrentPhaseState == EPhaseState::EnemyMovePhase)
	{
		// MoveConsumed 태그를 제거한 후 AIController의 SelectAbility 로직이 시작될 수 있도록, 순서 보장을 위해 분리된 콜백을 호출합니다.
		OnRoundStartedDelegate.Broadcast();
		PendingMoveEnemies = RegisteredEnemies;
	}
	
	OnChangePhaseStateDelegate.Broadcast(OldPhase, CurrentPhaseState);
	
	if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		AbilityResolverComponent->SortEnemyAbilityActivationData();
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

void ALetheGameState::RebuildCurrentInBattleEnemies()
{
	CurrentInBattleEnemies.Reset();
	for (const auto& Enemy : RegisteredEnemies)
	{
		if (!Enemy.IsValid())
		{
			continue;
		}
		
		const APawn* EnemyPawn = Cast<APawn>(Enemy.Get());
		if (!EnemyPawn)
		{
			continue;
		}

		if (ALetheAIController* AIController = EnemyPawn->GetController<ALetheAIController>())
		{
			if (AIController->IsPlayerCharacterInDetectionRange())
			{
				CurrentInBattleEnemies.Emplace(Enemy.Get());
			}
		}
	}
}

EPhaseState ALetheGameState::GetPhaseState() const
{
	return CurrentPhaseState;
}

void ALetheGameState::AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddPlayerAbilityActivationData(ActivationData);
}

void ALetheGameState::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddEnemyAbilityActivationData(ActivationData);
}

void ALetheGameState::OnEnemyAbilityActivated(AActor* AbilityInstigator) const
{
	OnActivateEnemyAbilityDelegate.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityEnded(const bool bSuccess) const
{
	AbilityResolverComponent->OnAbilityEnded(bSuccess);
}

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsProgressingPlayerAbility() const
{
	return AbilityResolverComponent->IsActivatingPlayerAbility();
}
