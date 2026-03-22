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
	
	AbilityResolverComponent->OnEnemyAbilityActivated.AddUObject(this, &ThisClass::OnEnemyAbilityActivated);
	AbilityResolverComponent->OnEnemyActivationQueueFinished.BindUObject(this, &ThisClass::OnEnemyExecutionQueueFinished);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->SetDummyActor(nullptr);
	AbilityResolverComponent->OnEnemyAbilityActivated.RemoveAll(this);
	AbilityResolverComponent->OnEnemyActivationQueueFinished.Unbind();
	
	Super::EndPlay(EndPlayReason);
}

void ALetheGameState::RegisterEnemy(AEnemyCharacterBase* Enemy)
{
	RegisteredEnemies.Emplace(Enemy);
}

void ALetheGameState::GoEnemyPlanningPhase()
{
	SetPhase(EPhaseState::EnemyPlanningPhase);
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
	
	OnChangePhaseStateDelegate.Broadcast(OldPhase, CurrentPhaseState);

	if (CurrentPhaseState == EPhaseState::EnemyPlanningPhase)
	{
		RegisteredEnemies.Sort([](const TWeakObjectPtr<AEnemyCharacterBase>& A, const TWeakObjectPtr<AEnemyCharacterBase>& B)
			{
				if (A.IsValid() && B.IsValid())
				{
					return A->GetEnemyAbilityPriority() < B->GetEnemyAbilityPriority();
				}
				return false;
			});
		
		CurrentEnemyIndex = 0;
		ProcessCurrentEnemyPlan();
	}
	
	if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		AbilityResolverComponent->SetEnemyAbilityActivationData(MoveTemp(ReservedEnemyAbilityActivationData));		
		AbilityResolverComponent->SortEnemyAbilityActivationData();
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

void ALetheGameState::ProcessCurrentEnemyPlan()
{
	if (!RegisteredEnemies.IsValidIndex(CurrentEnemyIndex))
	{
		// 모든 Enemy AI가 Plan을 마친 경우 들어오는 분기입니다.
		GoDrawPhase();
		return;
	}

	const TWeakObjectPtr<AEnemyCharacterBase>& CurrentEnemy = RegisteredEnemies[CurrentEnemyIndex];
	if (!CurrentEnemy.IsValid())
	{
		++CurrentEnemyIndex;
		ProcessCurrentEnemyPlan();
		return;
	}

	CurrentEnemy->ProcessPlanPhase();
}

void ALetheGameState::OnEnemyExecutionQueueFinished()
{
	GoEnemyPlanningPhase();
}

EPhaseState ALetheGameState::GetPhaseState() const
{
	return CurrentPhaseState;
}

void ALetheGameState::AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddPlayerAbilityActivationData(ActivationData);
}

void ALetheGameState::ActivateEnemyAbility(FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->ActivateEnemyAbility(ActivationData);
}

void ALetheGameState::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	ReservedEnemyAbilityActivationData.Emplace(ActivationData);
}

void ALetheGameState::OnEnemyAbilityActivated(AActor* AbilityInstigator) const
{
	OnActivateEnemyAbilityDelegate.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityActivationFailed()
{
	AbilityResolverComponent->OnAbilityActivationFailed();
}

void ALetheGameState::OnEnemyPlanMoveResolved()
{
	if (RegisteredEnemies.IsValidIndex(CurrentEnemyIndex))
	{
		RegisteredEnemies[CurrentEnemyIndex]->ProcessTelegraphPlan();
	}

	// Ability 사용 예고 직후 다른 Enemy가 너무 빨리 움직이지 않도록 타이머로 딜레이시킵니다.
	if (PlanTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(PlanTimerHandle);
	}
	GetWorld()->GetTimerManager().SetTimer(PlanTimerHandle,
		[this]()
		{
			++CurrentEnemyIndex;
			ProcessCurrentEnemyPlan();
		}, EnemyAbilityDelayTime, false);
}

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsProgressingPlayerAbility() const
{
	return AbilityResolverComponent->IsActivatingPlayerAbility();
}
