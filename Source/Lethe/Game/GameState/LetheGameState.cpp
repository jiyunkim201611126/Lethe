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
	
	AbilityResolverComponent->OnActivateEnemyAbility.BindUObject(this, &ThisClass::OnActivateEnemyAbility);
	AbilityResolverComponent->OnFinishActivationQueue.BindUObject(this, &ThisClass::OnFinishActivationQueue);
	AbilityResolverComponent->OnResolveUseCard.BindUObject(this, &ThisClass::OnResolveUseCard);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->SetDummyActor(nullptr);
	AbilityResolverComponent->OnActivateEnemyAbility.Unbind();
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
		GoEnemyPlanningPhase();
	}
	
	if (CurrentPhaseState == EPhaseState::EnemyTurnPhase)
	{
		GoEnemyPlanningPhase();
	}
}

bool ALetheGameState::HasAnyCombatEnemy() const
{
	return !CurrentCombatEnemies.IsEmpty();
}

void ALetheGameState::TryGoEnemyPlanningPhase()
{
	if (bShouldDeferEndPlayerMovePhase)
	{
		bShouldDeferEndPlayerMovePhase = false;
		return;
	}
	GoEnemyPlanningPhase();
}

EPhaseState ALetheGameState::GetPhaseState() const
{
	return CurrentPhaseState;
}

void ALetheGameState::EnqueuePlayerAbilityActivationData(FAbilityActivationData&& ActivationData, const bool bStartImmediately) const
{
	AbilityResolverComponent->EnqueuePlayerAbilityActivationData(MoveTemp(ActivationData), bStartImmediately);
}

void ALetheGameState::OnResolveUseCard(const int32 HandIndex, const bool bSuccess) const
{
	OnCardUseResolved.ExecuteIfBound(HandIndex, bSuccess);
}

void ALetheGameState::StartActivatePlayerMoveAbilities() const
{
	AbilityResolverComponent->StartActivatePlayerAbility();
}

void ALetheGameState::EnqueueEnemyAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	ReservedEnemyAbilityActivationData.Add(ActivationData);
}

void ALetheGameState::ActivateAbility(FAbilityActivationData& ActivationData, const ETeamSide TeamSide) const
{
	AbilityResolverComponent->ActivateAbility(ActivationData, TeamSide);
}

void ALetheGameState::OnActivateEnemyAbility(AActor* AbilityInstigator) const
{
	OnEnemyAbilityActivated.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityActivationFailed() const
{
	AbilityResolverComponent->OnAbilityActivationFailed();
}

void ALetheGameState::OnResolvePlayerMove(AActor* MovedCharacter) const
{
	OnPlayerMoveResolved.ExecuteIfBound(MovedCharacter);
}

void ALetheGameState::OnResolveEnemyPlanMove()
{
	LETHE_LOG(LogLetheGameState, Log, "On Enemy Plan Move Resolved");
	if (SpawnedEnemies.IsValidIndex(CurrentEnemyAbilityProcessIndex))
	{
		const TWeakObjectPtr<AEnemyCharacterBase>& AbilityProcessedEnemy = SpawnedEnemies[CurrentEnemyAbilityProcessIndex];
		if (AbilityProcessedEnemy.IsValid())
		{
			AbilityProcessedEnemy->ProcessTelegraphPlan();
		}
	}

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
	if (CurrentPhaseState == EPhaseState::EnemyPlanningPhase)
	{
		return HasAnyCombatEnemy();
	}

	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		return false;
	}

	return true;
}
