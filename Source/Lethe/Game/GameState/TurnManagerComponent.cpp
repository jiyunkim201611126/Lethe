// Copyright JETBLU, Inc. All Rights Reserved.

#include "TurnManagerComponent.h"

#include "AbilityResolverComponent.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Interface/CombatInterface.h"
#include "TimerManager.h"

UTurnManagerComponent::UTurnManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTurnManagerComponent::Initialize(UAbilityResolverComponent* InAbilityResolverComponent)
{
	if (AbilityResolverComponent == InAbilityResolverComponent)
	{
		return;
	}

	Deinitialize();

	AbilityResolverComponent = InAbilityResolverComponent;
	if (AbilityResolverComponent)
	{
		AbilityResolverComponent->OnFinishActivationQueue.BindUObject(this, &ThisClass::NotifyAbilityQueueCompleted);
	}
}

void UTurnManagerComponent::Deinitialize()
{
	if (AbilityResolverComponent)
	{
		AbilityResolverComponent->OnFinishActivationQueue.Unbind();
		AbilityResolverComponent = nullptr;
	}

	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PlanTimerHandle);
	}
}

void UTurnManagerComponent::RegisterPlayerCharacter(AActor* PlayerCharacter)
{
	PlayerCharacters.AddUnique(PlayerCharacter);
}

void UTurnManagerComponent::RegisterEnemy(AEnemyCharacterBase* Enemy)
{
	SpawnedEnemies.Add(Enemy);
}

void UTurnManagerComponent::RegisterCombatEnemy(AEnemyCharacterBase* Enemy)
{
	if (!CurrentCombatEnemies.Contains(Enemy))
	{
		CurrentCombatEnemies.Add(Enemy);
	}
}

void UTurnManagerComponent::UnregisterEnemy(AEnemyCharacterBase* Enemy)
{
	SpawnedEnemies.Remove(Enemy);
	ReservedEnemyAbilityActivationContexts.RemoveAll([Enemy](const FAbilityActivationContext& ActivationContext)
	{
		return ActivationContext.Index == Enemy->GetEnemyAbilityPriority();
	});
	CurrentCombatEnemies.Remove(Enemy);
}

void UTurnManagerComponent::StartTurnFlow()
{
	TryTransitionToPhase(EPhaseState::EnemyPlanPhase);
}

void UTurnManagerComponent::RequestEndPlayerMovePhase()
{
	if (CurrentPhaseState != EPhaseState::PlayerMovePhase)
	{
		return;
	}

	if (bShouldDeferEndPlayerMovePhase)
	{
		bShouldDeferEndPlayerMovePhase = false;
		return;
	}

	TryTransitionToPhase(EPhaseState::EnemyPlanPhase);
}

void UTurnManagerComponent::NotifyDrawPhaseCompleted()
{
	TryTransitionToPhase(EPhaseState::PlayerTurnPhase);
}

void UTurnManagerComponent::RequestEndPlayerTurn()
{
	if (!AbilityResolverComponent || AbilityResolverComponent->IsResolvingPlayerAbility())
	{
		return;
	}

	TryTransitionToPhase(EPhaseState::EnemyTurnPhase);
}

void UTurnManagerComponent::NotifyAbilityQueueCompleted()
{
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		for (const auto& PlayerCharacter : PlayerCharacters)
		{
			if (PlayerCharacter.GetInterface() && 0 < PlayerCharacter->GetMoveRange())
			{
				// 잔여 행동력이 있다면 이번 입력에 턴 종료를 수행해선 안 됩니다.
				// TODO: 잔여 행동력(MoveRange)이 있다고 알림
				bShouldDeferEndPlayerMovePhase = false;
				return;
			}
		}

		bShouldDeferEndPlayerMovePhase = false;
		TryTransitionToPhase(EPhaseState::EnemyPlanPhase);
		return;
	}

	if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		TryTransitionToPhase(EPhaseState::EnemyPlanPhase);
	}
}

void UTurnManagerComponent::NotifyCardUseResolved()
{
	// PlayerMovePhase에 카드를 사용했고, 전투 중인 적이 하나라도 있다면 DrawPhase로 직행합니다.
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase && HasAnyCombatEnemy())
	{
		TryTransitionToPhase(EPhaseState::DrawPhase);
	}
}

void UTurnManagerComponent::NotifyEnemyPlanResolved()
{
	if (CurrentPhaseState != EPhaseState::EnemyPlanPhase)
	{
		return;
	}

	LETHE_LOG(LogLetheGameState, Log, "On Enemy Plan Move Resolved");

	// Plan 종료 후 다른 Enemy가 너무 빨리 움직이지 않도록 타이머로 딜레이시킵니다.
	if (PlanTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(PlanTimerHandle);
	}
	GetWorld()->GetTimerManager().SetTimer(PlanTimerHandle, this, &ThisClass::OnPlanTimerEnded, EnemyAbilityDelayTime, false);
}

void UTurnManagerComponent::NotifyPlayerMovePlanChanged()
{
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		bShouldDeferEndPlayerMovePhase = true;
	}
}

void UTurnManagerComponent::EnqueueEnemyAbilityActivationContext(const FAbilityActivationContext& ActivationContext)
{
	ReservedEnemyAbilityActivationContexts.Add(ActivationContext);
}

bool UTurnManagerComponent::IsBattlePhase() const
{
	if (CurrentPhaseState == EPhaseState::EnemyPlanPhase)
	{
		return HasAnyCombatEnemy();
	}

	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		return false;
	}

	return true;
}

TArray<AActor*> UTurnManagerComponent::GetPlayerCharacters() const
{
	TArray<AActor*> TempPlayerCharacters;
	for (const auto& PlayerCharacter : PlayerCharacters)
	{
		TempPlayerCharacters.Add(Cast<AActor>(PlayerCharacter.GetObject()));
	}
	return TempPlayerCharacters;
}

bool UTurnManagerComponent::TryTransitionToPhase(const EPhaseState NewPhaseState)
{
	const EPhaseState OldPhaseState = CurrentPhaseState;
	if (!CanTransitionToPhase(NewPhaseState))
	{
		LETHE_LOG(LogLetheGameState, Warning, "Rejected Phase Transition: %s -> %s", *UEnum::GetValueAsString(OldPhaseState), *UEnum::GetValueAsString(NewPhaseState));
		return false;
	}

	CurrentPhaseState = NewPhaseState;
	OnChangePhaseState.Broadcast(OldPhaseState, CurrentPhaseState);
	EnterPhase(CurrentPhaseState);
	return true;
}

bool UTurnManagerComponent::CanTransitionToPhase(const EPhaseState NewPhaseState) const
{
	switch (CurrentPhaseState)
	{
	case EPhaseState::None:
		return NewPhaseState == EPhaseState::EnemyPlanPhase;
	case EPhaseState::EnemyPlanPhase:
		return NewPhaseState == EPhaseState::PlayerMovePhase || NewPhaseState == EPhaseState::DrawPhase;
	case EPhaseState::PlayerMovePhase:
		return NewPhaseState == EPhaseState::EnemyPlanPhase || NewPhaseState == EPhaseState::DrawPhase;
	case EPhaseState::DrawPhase:
		return NewPhaseState == EPhaseState::PlayerTurnPhase;
	case EPhaseState::PlayerTurnPhase:
		return NewPhaseState == EPhaseState::EnemyTurnPhase;
	case EPhaseState::EnemyTurnPhase:
		return NewPhaseState == EPhaseState::EnemyPlanPhase;
	default:
		return false;
	}
}

void UTurnManagerComponent::EnterPhase(const EPhaseState PhaseState)
{
	if (PhaseState == EPhaseState::EnemyPlanPhase)
	{
		StartEnemyPlanPhase();
		return;
	}

	if (PhaseState == EPhaseState::EnemyTurnPhase)
	{
		StartEnemyTurnPhase();
	}
}

void UTurnManagerComponent::StartEnemyPlanPhase()
{
	SpawnedEnemies.RemoveAll([](const TWeakObjectPtr<AEnemyCharacterBase>& Enemy)
	{
		return !Enemy.IsValid();
	});
	SpawnedEnemies.Sort([](const TWeakObjectPtr<AEnemyCharacterBase>& A, const TWeakObjectPtr<AEnemyCharacterBase>& B)
	{
		return A->GetEnemyAbilityPriority() < B->GetEnemyAbilityPriority();
	});

	CurrentEnemyAbilityProcessIndex = 0;
	ProcessCurrentEnemyPlan();
}

void UTurnManagerComponent::StartEnemyTurnPhase()
{
	if (!AbilityResolverComponent)
	{
		return;
	}

	AbilityResolverComponent->SetEnemyAbilityActivationContext(MoveTemp(ReservedEnemyAbilityActivationContexts));
	AbilityResolverComponent->StartActivateEnemyAbility();
}

void UTurnManagerComponent::ProcessCurrentEnemyPlan()
{
	if (!SpawnedEnemies.IsValidIndex(CurrentEnemyAbilityProcessIndex))
	{
		// 모든 Enemy AI가 Plan을 마친 경우 들어오는 분기입니다.
		TryTransitionToPhase(HasAnyCombatEnemy() ? EPhaseState::DrawPhase : EPhaseState::PlayerMovePhase);
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

void UTurnManagerComponent::OnPlanTimerEnded()
{
	if (CurrentPhaseState != EPhaseState::EnemyPlanPhase)
	{
		return;
	}

	++CurrentEnemyAbilityProcessIndex;
	ProcessCurrentEnemyPlan();
}

bool UTurnManagerComponent::HasAnyCombatEnemy() const
{
	for (const auto& CombatEnemy : CurrentCombatEnemies)
	{
		if (CombatEnemy.IsValid())
		{
			return true;
		}
	}
	return false;
}
