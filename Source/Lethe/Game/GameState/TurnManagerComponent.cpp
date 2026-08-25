// Copyright JETBLU, Inc. All Rights Reserved.

#include "TurnManagerComponent.h"

#include "AbilityResolverComponent.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Interface/CombatInterface.h"
#include "TimerManager.h"
#include "Lethe/Util.h"

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
	TryTransitionToTurnPhaseState(ETurnPhaseState::EnemyPlanPhase);
}

void UTurnManagerComponent::RequestEndPlayerMovePhase()
{
	if (CurrentTurnPhaseState != ETurnPhaseState::PlayerMovePhase)
	{
		return;
	}

	if (bShouldDeferEndPlayerMovePhase)
	{
		bShouldDeferEndPlayerMovePhase = false;
		return;
	}

	TryTransitionToTurnPhaseState(ETurnPhaseState::EnemyPlanPhase);
}

void UTurnManagerComponent::NotifyDrawPhaseCompleted()
{
	TryTransitionToTurnPhaseState(ETurnPhaseState::PlayerTurnPhase);
}

void UTurnManagerComponent::RequestEndPlayerTurn()
{
	if (!AbilityResolverComponent || AbilityResolverComponent->IsResolvingPlayerAbility())
	{
		return;
	}

	TryTransitionToTurnPhaseState(ETurnPhaseState::EnemyTurnPhase);
}

void UTurnManagerComponent::NotifyAbilityQueueCompleted()
{
	if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
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
		TryTransitionToTurnPhaseState(ETurnPhaseState::EnemyPlanPhase);
		return;
	}

	if (CurrentTurnPhaseState == ETurnPhaseState::EnemyTurnPhase)
	{
		TryTransitionToTurnPhaseState(ETurnPhaseState::EnemyPlanPhase);
	}
}

void UTurnManagerComponent::NotifyCardUseResolved()
{
	// PlayerMovePhase에 카드를 사용했고, 전투 중인 적이 하나라도 있다면 DrawPhase로 직행합니다.
	if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase && HasAnyCombatEnemy())
	{
		TryTransitionToTurnPhaseState(ETurnPhaseState::DrawPhase);
	}
}

void UTurnManagerComponent::NotifyEnemyPlanResolved()
{
	if (CurrentTurnPhaseState != ETurnPhaseState::EnemyPlanPhase)
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
	if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
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
	if (CurrentTurnPhaseState == ETurnPhaseState::EnemyPlanPhase)
	{
		return HasAnyCombatEnemy();
	}

	if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
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

bool UTurnManagerComponent::TryTransitionToTurnPhaseState(const ETurnPhaseState NewTurnPhaseState)
{
	const ETurnPhaseState OldTurnPhaseState = CurrentTurnPhaseState;
	if (!CanTransitionToTurnPhaseState(NewTurnPhaseState))
	{
		LETHE_LOG(LogLetheGameState, Warning, "Rejected Phase Transition: %s -> %s", *UEnum::GetValueAsString(OldTurnPhaseState), *UEnum::GetValueAsString(NewTurnPhaseState));
		return false;
	}

	CurrentTurnPhaseState = NewTurnPhaseState;
	OnTurnPhaseStateChanged.Broadcast(OldTurnPhaseState, CurrentTurnPhaseState);
	EnterTurnPhaseState(CurrentTurnPhaseState);
	return true;
}

bool UTurnManagerComponent::CanTransitionToTurnPhaseState(const ETurnPhaseState NewTurnPhaseState) const
{
	switch (CurrentTurnPhaseState)
	{
	case ETurnPhaseState::None:
		return NewTurnPhaseState == ETurnPhaseState::EnemyPlanPhase;
	case ETurnPhaseState::EnemyPlanPhase:
		return NewTurnPhaseState == ETurnPhaseState::PlayerMovePhase || NewTurnPhaseState == ETurnPhaseState::DrawPhase;
	case ETurnPhaseState::PlayerMovePhase:
		return NewTurnPhaseState == ETurnPhaseState::EnemyPlanPhase || NewTurnPhaseState == ETurnPhaseState::DrawPhase;
	case ETurnPhaseState::DrawPhase:
		return NewTurnPhaseState == ETurnPhaseState::PlayerTurnPhase;
	case ETurnPhaseState::PlayerTurnPhase:
		return NewTurnPhaseState == ETurnPhaseState::EnemyTurnPhase;
	case ETurnPhaseState::EnemyTurnPhase:
		return NewTurnPhaseState == ETurnPhaseState::EnemyPlanPhase;
	default:
		return false;
	}
}

void UTurnManagerComponent::EnterTurnPhaseState(const ETurnPhaseState TurnPhaseState)
{
	if (TurnPhaseState == ETurnPhaseState::EnemyPlanPhase)
	{
		StartEnemyPlanPhase();
		return;
	}

	if (TurnPhaseState == ETurnPhaseState::EnemyTurnPhase)
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
		TryTransitionToTurnPhaseState(HasAnyCombatEnemy() ? ETurnPhaseState::DrawPhase : ETurnPhaseState::PlayerMovePhase);
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
	if (CurrentTurnPhaseState != ETurnPhaseState::EnemyPlanPhase)
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

#if WITH_EDITOR
void UTurnManagerComponent::AppendDebugSnapshot(FStringBuilderBase& Builder) const
{
	int32 ValidPlayerCount = 0;
	for (const TScriptInterface<ICombatInterface>& PlayerCharacter : PlayerCharacters)
	{
		ValidPlayerCount += IsValid(PlayerCharacter.GetObject()) ? 1 : 0;
	}

	int32 ValidEnemyCount = 0;
	for (const TWeakObjectPtr<AEnemyCharacterBase>& Enemy : SpawnedEnemies)
	{
		ValidEnemyCount += Enemy.IsValid() ? 1 : 0;
	}

	int32 ValidCombatEnemyCount = 0;
	for (const TWeakObjectPtr<AEnemyCharacterBase>& Enemy : CurrentCombatEnemies)
	{
		ValidCombatEnemyCount += Enemy.IsValid() ? 1 : 0;
	}

	const AEnemyCharacterBase* CurrentEnemy = SpawnedEnemies.IsValidIndex(CurrentEnemyAbilityProcessIndex)
		? SpawnedEnemies[CurrentEnemyAbilityProcessIndex].Get()
		: nullptr;
	const UWorld* World = GetWorld();
	const bool bPlanTimerActive = World && World->GetTimerManager().IsTimerActive(PlanTimerHandle);
	const float PlanTimerRemaining = bPlanTimerActive ? World->GetTimerManager().GetTimerRemaining(PlanTimerHandle) : -1.f;

	Builder.Append(TEXT("\n[TurnManagerComponent]\n"));
	Builder.Appendf(TEXT("  TurnPhaseState = %s\n"), *LogHelper::EnumToString(CurrentTurnPhaseState));
	Builder.Appendf(TEXT("  Players = %d/%d, SpawnedEnemies = %d/%d, CombatEnemies = %d/%d\n"),
		ValidPlayerCount, PlayerCharacters.Num(),
		ValidEnemyCount, SpawnedEnemies.Num(),
		ValidCombatEnemyCount, CurrentCombatEnemies.Num());
	Builder.Appendf(TEXT("  EnemyPlanIndex = %d/%d, CurrentEnemy = %s, AIController = %s\n"),
		CurrentEnemyAbilityProcessIndex, SpawnedEnemies.Num(),
		CurrentEnemy ? *GetNameSafe(CurrentEnemy) : TEXT("nullptr"),
		CurrentEnemy && CurrentEnemy->GetController() ? *GetNameSafe(CurrentEnemy->GetController()) : TEXT("nullptr"));
	Builder.Appendf(TEXT("  PlanTimerActive = %s, PlanTimerRemaining = %.3f초\n"),
		bPlanTimerActive ? TEXT("true") : TEXT("false"), PlanTimerRemaining);
	Builder.Appendf(TEXT("  ReservedEnemyAbilities = %d, DeferPlayerMoveEnd = %s\n"),
		ReservedEnemyAbilityActivationContexts.Num(), bShouldDeferEndPlayerMovePhase ? TEXT("true") : TEXT("false"));

	if (CurrentTurnPhaseState == ETurnPhaseState::EnemyPlanPhase)
	{
		if (bPlanTimerActive)
		{
			Builder.Append(TEXT("  InferredProgress = 적 계획 지연 타이머 대기\n"));
		}
		else if (CurrentEnemy)
		{
			Builder.Appendf(TEXT("  InferredProgress = 적 계획 완료 대기 (%s)\n"), *GetNameSafe(CurrentEnemy));
		}
		else
		{
			Builder.Append(TEXT("  InferredProgress = 현재 처리할 적 포인터가 nullptr인 상태\n"));
		}
	}
	else if (CurrentTurnPhaseState == ETurnPhaseState::EnemyTurnPhase)
	{
		Builder.Append(TEXT("  InferredProgress = Enemy Ability 완료 대기\n"));
	}
	else if (CurrentTurnPhaseState == ETurnPhaseState::DrawPhase)
	{
		Builder.Append(TEXT("  InferredProgress = Draw 완료 대기\n"));
	}
	else if (CurrentTurnPhaseState == ETurnPhaseState::PlayerTurnPhase)
	{
		Builder.Append(TEXT("  InferredProgress = 플레이어 턴 종료 또는 Ability 완료 대기\n"));
	}
	else if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
	{
		Builder.Append(AbilityResolverComponent && AbilityResolverComponent->IsResolvingPlayerAbility()
			? TEXT("  InferredProgress = 플레이어 MoveAbility 완료 대기\n")
			: TEXT("  InferredProgress = 플레이어 Move 입력 또는 종료 요청 대기\n"));
	}
	else
	{
		Builder.Append(TEXT("  InferredProgress = 턴 흐름 시작 전\n"));
	}

	for (int32 Index = 0; Index < ReservedEnemyAbilityActivationContexts.Num(); ++Index)
	{
		const FAbilityActivationContext& Context = ReservedEnemyAbilityActivationContexts[Index];
		Builder.Appendf(TEXT("    ReservedAbility[%d]: Priority = %d, Tag = %s, SpecValid = %s, OwnerASCValid = %s, TargetResults = %d, PathTiles = %d\n"),
			Index, Context.Index, *Context.AbilityTag.ToString(),
			Context.AbilitySpecHandle.IsValid() ? TEXT("true") : TEXT("false"),
			Context.AbilityOwnerASC.IsValid() ? TEXT("true") : TEXT("false"),
			Context.TargetSelectionResults.Num(), Context.PathTiles.Num());
	}

	Builder.Append(TEXT("  검증 결과:\n"));
	int32 IssueCount = 0;
	auto AppendIssue = [&Builder, &IssueCount](const TCHAR* Issue)
	{
		++IssueCount;
		Builder.Appendf(TEXT("    [!] %s\n"), Issue);
	};

	if (ValidPlayerCount != PlayerCharacters.Num())
	{
		AppendIssue(TEXT("PlayerCharacters에 유효하지 않은 항목이 있습니다."));
	}
	if (ValidEnemyCount != SpawnedEnemies.Num())
	{
		AppendIssue(TEXT("SpawnedEnemies에 유효하지 않은 항목이 있습니다."));
	}
	if (ValidCombatEnemyCount != CurrentCombatEnemies.Num())
	{
		AppendIssue(TEXT("CurrentCombatEnemies에 유효하지 않은 항목이 있습니다."));
	}
	if (CurrentTurnPhaseState == ETurnPhaseState::EnemyPlanPhase && !bPlanTimerActive && !CurrentEnemy)
	{
		AppendIssue(TEXT("EnemyPlanPhase인데 현재 처리할 적도, 작동 중인 계획 지연 타이머도 없습니다."));
	}
	if (CurrentTurnPhaseState != ETurnPhaseState::EnemyPlanPhase && bPlanTimerActive)
	{
		AppendIssue(TEXT("EnemyPlanPhase가 아닌데 적 계획 타이머가 작동 중입니다."));
	}
	if (CurrentTurnPhaseState == ETurnPhaseState::EnemyTurnPhase && !ReservedEnemyAbilityActivationContexts.IsEmpty())
	{
		AppendIssue(TEXT("EnemyTurnPhase인데 TurnManager에 예약된 Enemy Ability가 남아 있습니다."));
	}

	if (IssueCount == 0)
	{
		Builder.Append(TEXT("    명백한 상태 불일치를 찾지 못했습니다.\n"));
	}
}
#endif
