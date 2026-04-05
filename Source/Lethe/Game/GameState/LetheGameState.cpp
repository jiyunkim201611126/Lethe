// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

#include "Lethe/Character/EnemyCharacterBase.h"

ALetheGameState::ALetheGameState()
{
	AbilityResolverComponent = CreateDefaultSubobject<UAbilityResolverComponent>("AbilityResolverComponent");
}

void ALetheGameState::BeginPlay()
{
	Super::BeginPlay();

	check(DummyActorClass);
	if (AActor* DummyActor = GetWorld()->SpawnActor<AActor>(DummyActorClass))
	{
		AbilityResolverComponent->SetDummyActor(DummyActor);
	}
	
	AbilityResolverComponent->OnActivateEnemyAbility.BindUObject(this, &ThisClass::OnActivateEnemyAbility);
	AbilityResolverComponent->OnFinishActivationQueue.BindUObject(this, &ThisClass::OnFinishActivationQueue);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->SetDummyActor(nullptr);
	AbilityResolverComponent->OnActivateEnemyAbility.Unbind();
	AbilityResolverComponent->OnFinishActivationQueue.Unbind();
	
	Super::EndPlay(EndPlayReason);
}

void ALetheGameState::RegisterEnemy(AEnemyCharacterBase* Enemy)
{
	SpawnedEnemies.Emplace(Enemy);
}

void ALetheGameState::RegisterCombatEnemy(AEnemyCharacterBase* Enemy)
{
	if (!CurrentCombatEnemies.Contains(Enemy))
	{
		CurrentCombatEnemies.Emplace(Enemy);
	}
}

void ALetheGameState::UnregisterEnemy(AEnemyCharacterBase* Enemy)
{
	SpawnedEnemies.Remove(Enemy);
	ReservedEnemyAbilityActivationData.RemoveAll([Enemy](const FAbilityActivationData& ActivationData)
	{
		return ActivationData.Index == Enemy->GetEnemyAbilityPriority();
	});
	CurrentCombatEnemies.Remove(Enemy);
}

void ALetheGameState::GoEnemyPlanningPhase()
{
	SetPhase(EPhaseState::EnemyPlanningPhase);
}

void ALetheGameState::GoPlayerMovePhase()
{
	SetPhase(EPhaseState::PlayerMovePhase);
}

void ALetheGameState::GoDrawPhase()
{
	SetPhase(EPhaseState::DrawPhase);
}

void ALetheGameState::GoPlayerTurnPhase()
{
	SetPhase(EPhaseState::PlayerTurnPhase);
}

void ALetheGameState::GoEnemyTurnPhase()
{
	SetPhase(EPhaseState::EnemyTurnPhase);
}

void ALetheGameState::SetPhase(const EPhaseState NewPhase)
{
	const EPhaseState OldPhase = CurrentPhaseState;
	if (OldPhase == NewPhase)
	{
		return;
	}
	
	CurrentPhaseState = NewPhase;
	
	OnChangePhaseState.Broadcast(OldPhase, CurrentPhaseState);

	if (CurrentPhaseState == EPhaseState::EnemyPlanningPhase)
	{
		SpawnedEnemies.Sort([](const TWeakObjectPtr<AEnemyCharacterBase>& A, const TWeakObjectPtr<AEnemyCharacterBase>& B)
			{
				if (A.IsValid() && B.IsValid())
				{
					return A->GetEnemyAbilityPriority() < B->GetEnemyAbilityPriority();
				}
				return false;
			});
		
		CurrentEnemyAbilityProcessIndex = 0;
		ProcessCurrentEnemyPlan();
	}
	
	if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		AbilityResolverComponent->SetEnemyAbilityActivationData(MoveTemp(ReservedEnemyAbilityActivationData));
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

void ALetheGameState::ProcessCurrentEnemyPlan()
{
	if (!SpawnedEnemies.IsValidIndex(CurrentEnemyAbilityProcessIndex))
	{
		// 모든 Enemy AI가 Plan을 마친 경우 들어오는 분기입니다.
		ShouldGoCombatPhase() ? GoDrawPhase() : GoPlayerMovePhase();
		return;
	}

	const TWeakObjectPtr<AEnemyCharacterBase>& CurrentEnemy = SpawnedEnemies[CurrentEnemyAbilityProcessIndex];
	if (!CurrentEnemy.IsValid())
	{
		++CurrentEnemyAbilityProcessIndex;
		ProcessCurrentEnemyPlan();
		return;
	}

	CurrentEnemy->ProcessPlanPhase();
}

void ALetheGameState::OnFinishActivationQueue()
{
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase || CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		GoEnemyPlanningPhase();
	}
}

bool ALetheGameState::ShouldGoCombatPhase() const
{
	return !CurrentCombatEnemies.IsEmpty();
}

EPhaseState ALetheGameState::GetPhaseState() const
{
	return CurrentPhaseState;
}

void ALetheGameState::AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddPlayerAbilityActivationData(ActivationData);
}

void ALetheGameState::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	ReservedEnemyAbilityActivationData.Emplace(ActivationData);
}

void ALetheGameState::ActivateAbility(FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->ActivateAbility(ActivationData);
}

void ALetheGameState::OnActivateEnemyAbility(AActor* AbilityInstigator) const
{
	OnEnemyAbilityActivated.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityActivationFailed() const
{
	AbilityResolverComponent->OnAbilityActivationFailed();
}

void ALetheGameState::OnResolvePlayerMove(const AActor* MovedCharacter) const
{
	OnPlayerMoveResolved.ExecuteIfBound(MovedCharacter);
}

void ALetheGameState::OnResolveEnemyPlanMove()
{
	if (SpawnedEnemies.IsValidIndex(CurrentEnemyAbilityProcessIndex))
	{
		SpawnedEnemies[CurrentEnemyAbilityProcessIndex]->ProcessTelegraphPlan();
	}

	// Ability 사용 예고 직후 다른 Enemy가 너무 빨리 움직이지 않도록 타이머로 딜레이시킵니다.
	if (PlanTimerHandle.IsValid())
	{
		GetWorldTimerManager().ClearTimer(PlanTimerHandle);
	}
	GetWorldTimerManager().SetTimer(PlanTimerHandle, this, &ThisClass::OnPlanTimerEnded, EnemyAbilityDelayTime, false);
}

void ALetheGameState::OnPlanTimerEnded()
{
	++CurrentEnemyAbilityProcessIndex;
	ProcessCurrentEnemyPlan();
}

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsProgressingPlayerAbility() const
{
	return AbilityResolverComponent->IsActivatingPlayerAbility();
}
