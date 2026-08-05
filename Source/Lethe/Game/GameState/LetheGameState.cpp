// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

#include "AbilityResolverComponent.h"
#include "Lethe/LetheLog.h"
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
	
	AbilityResolverComponent->OnAttemptEnemyAbility.BindUObject(this, &ThisClass::OnAttemptEnemyAbility);
	AbilityResolverComponent->OnFinishActivationQueue.BindUObject(this, &ThisClass::OnFinishActivationQueue);
	AbilityResolverComponent->OnResolveUseCard.BindUObject(this, &ThisClass::OnResolveUseCard);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->SetDummyActor(nullptr);
	AbilityResolverComponent->OnAttemptEnemyAbility.Unbind();
	AbilityResolverComponent->OnFinishActivationQueue.Unbind();
	
	Super::EndPlay(EndPlayReason);
}

void ALetheGameState::RegisterPlayerCharacter(AActor* PlayerCharacter)
{
	PlayerCharacters.AddUnique(PlayerCharacter);
}

void ALetheGameState::RegisterEnemy(AEnemyCharacterBase* Enemy)
{
	SpawnedEnemies.Add(Enemy);
}

void ALetheGameState::RegisterCombatEnemy(AEnemyCharacterBase* Enemy)
{
	if (!CurrentCombatEnemies.Contains(Enemy))
	{
		CurrentCombatEnemies.Add(Enemy);
	}
}

void ALetheGameState::UnregisterEnemy(AEnemyCharacterBase* Enemy)
{
	SpawnedEnemies.Remove(Enemy);
	ReservedEnemyAbilityActivationContext.RemoveAll([Enemy](const FAbilityActivationContext& ActivationContext)
	{
		return ActivationContext.Index == Enemy->GetEnemyAbilityPriority();
	});
	CurrentCombatEnemies.Remove(Enemy);
}

void ALetheGameState::GoEnemyPlanPhase()
{
	SetPhase(EPhaseState::EnemyPlanPhase);
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

void ALetheGameState::SetPhase(const EPhaseState NewPhaseState)
{
	const EPhaseState OldPhaseState = CurrentPhaseState;
	if (OldPhaseState == NewPhaseState)
	{
		return;
	}
	
	CurrentPhaseState = NewPhaseState;
	
	OnChangePhaseState.Broadcast(OldPhaseState, CurrentPhaseState);

	if (CurrentPhaseState == EPhaseState::EnemyPlanPhase)
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
		AbilityResolverComponent->SetEnemyAbilityActivationContext(MoveTemp(ReservedEnemyAbilityActivationContext));
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

void ALetheGameState::ProcessCurrentEnemyPlan()
{
	if (!SpawnedEnemies.IsValidIndex(CurrentEnemyAbilityProcessIndex))
	{
		// 모든 Enemy AI가 Plan을 마친 경우 들어오는 분기입니다.
		HasAnyCombatEnemy() ? GoDrawPhase() : GoPlayerMovePhase();
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
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		for (const auto& PlayerCharacter : PlayerCharacters)
		{
			if (0 < PlayerCharacter->GetMoveRange())
			{
				// 잔여 행동력이 있다면 이번 입력에 턴 종료를 수행해선 안 됩니다.
				// TODO: 잔여 행동력(MoveRange)이 있다고 알림
				bShouldDeferEndPlayerMovePhase = false;
				return;
			}
		}

		bShouldDeferEndPlayerMovePhase = false;
		GoEnemyPlanPhase();
	}
	
	if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		GoEnemyPlanPhase();
	}
}

bool ALetheGameState::HasAnyCombatEnemy() const
{
	return !CurrentCombatEnemies.IsEmpty();
}

void ALetheGameState::TryGoEnemyPlanPhase()
{
	if (bShouldDeferEndPlayerMovePhase)
	{
		bShouldDeferEndPlayerMovePhase = false;
		return;
	}
	GoEnemyPlanPhase();
}

EPhaseState ALetheGameState::GetPhaseState() const
{
	return CurrentPhaseState;
}

void ALetheGameState::EnqueuePlayerAbilityActivationContext(FAbilityActivationContext&& ActivationContext, const bool bStartImmediately) const
{
	AbilityResolverComponent->EnqueuePlayerAbilityActivationContext(MoveTemp(ActivationContext), bStartImmediately);
}

void ALetheGameState::OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess)
{
	OnCardUseResolved.ExecuteIfBound(HandSlotIndex, bSuccess);

	// PlayerMovePhase에 카드를 사용했고, 전투 중인 적이 하나라도 있다면 DrawPhase로 직행합니다.
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		if (HasAnyCombatEnemy())
		{
			GoDrawPhase();
		}
	}
}

void ALetheGameState::StartActivatePlayerMoveAbilities() const
{
	AbilityResolverComponent->StartActivatePlayerAbility();
}

void ALetheGameState::EnqueueEnemyAbilityActivationContext(const FAbilityActivationContext& ActivationContext)
{
	ReservedEnemyAbilityActivationContext.Add(ActivationContext);
}

void ALetheGameState::ActivateAbility(FAbilityActivationContext& ActivationContext, const ETeamSide TeamSide) const
{
	AbilityResolverComponent->ActivateAbility(ActivationContext, TeamSide);
}

void ALetheGameState::OnAttemptEnemyAbility(AActor* AbilityInstigator) const
{
	OnEnemyAbilityAttempt.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityActivationFailed() const
{
	AbilityResolverComponent->OnAbilityActivationFailed();
}

void ALetheGameState::NotifyPlayerMoveResolved(AActor* MovedCharacter) const
{
	OnPlayerMoveResolved.ExecuteIfBound(MovedCharacter);
}

void ALetheGameState::NotifyEnemyPlanResolved()
{
	LETHE_LOG(LogLetheGameState, Log, "On Enemy Plan Move Resolved");

	// Plan 종료 후 다른 Enemy가 너무 빨리 움직이지 않도록 타이머로 딜레이시킵니다.
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

void ALetheGameState::SetShouldDeferEndPlayerMovePhase()
{
	bShouldDeferEndPlayerMovePhase = true;
}

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsResolvingPlayerAbility() const
{
	return AbilityResolverComponent->IsResolvingPlayerAbility();
}

TArray<AActor*> ALetheGameState::GetPlayerCharacters() const
{
	TArray<AActor*> TempPlayerCharacters;
	for (const auto& PlayerCharacter : PlayerCharacters)
	{
		TempPlayerCharacters.Add(Cast<AActor>(PlayerCharacter.GetObject()));
	}
	return TempPlayerCharacters;
}

bool ALetheGameState::IsBattlePhase() const
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
