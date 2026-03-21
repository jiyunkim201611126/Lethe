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
	
	AbilityResolverComponent->OnEnemyAbilityActivated.AddUObject(this, &ThisClass::OnEnemyAbilityActivated);
	AbilityResolverComponent->OnAllEnemyAbilityResolved.BindUObject(this, &ThisClass::GoEnemyPlanningPhase);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->OnEnemyAbilityActivated.RemoveAll(this);
	AbilityResolverComponent->OnAllEnemyAbilityResolved.Unbind();
	
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
		AbilityResolverComponent->SortEnemyAbilityActivationData();
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

void ALetheGameState::ProcessCurrentEnemyPlan()
{
	if (!RegisteredEnemies.IsValidIndex(CurrentEnemyIndex))
	{
		// 모든 Enemy AI가 Plan을 마친 경우 들어오는 분기입니다.
		bIsEnemyPlanning = false;
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

	bIsEnemyPlanning = true;
	CurrentEnemy->ProcessPlanPhase();
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

void ALetheGameState::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddEnemyAbilityActivationData(ActivationData);
}

void ALetheGameState::OnEnemyAbilityActivated(AActor* AbilityInstigator) const
{
	OnActivateEnemyAbilityDelegate.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityEnded(const bool bSuccess)
{
	if (bIsEnemyPlanning)
	{
		if (bSuccess)
		{
			if (RegisteredEnemies.IsValidIndex(CurrentEnemyIndex))
			{
				RegisteredEnemies[CurrentEnemyIndex]->ProcessTelegraphPlan();
			}
		}

		// Ability 사용 예고 직후 다른 Enemy가 너무 빨리 움직이지 않도록 타이머로 딜레이시킵니다.
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle,
			[this]()
			{
				++CurrentEnemyIndex;
				ProcessCurrentEnemyPlan();
			}, 0.5f, false);
	}
	else
	{
		AbilityResolverComponent->OnAbilityEnded(bSuccess);
	}
}

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsProgressingPlayerAbility() const
{
	return AbilityResolverComponent->IsActivatingPlayerAbility();
}
