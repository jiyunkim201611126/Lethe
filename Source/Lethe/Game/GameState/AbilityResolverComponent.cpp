// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityResolverComponent.h"

#include "AbilitySystemComponent.h"
#include "Lethe/Lethe.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

UAbilityResolverComponent::UAbilityResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PlayerAbilityActivationContexts.Reserve(MAX_HAND_COUNT);
}

void UAbilityResolverComponent::SetDummyActor(AActor* InDummyActor)
{
	DummyActor = InDummyActor;
}

void UAbilityResolverComponent::EnqueuePlayerAbilityActivationContext(FAbilityActivationContext&& ActivationContext, const bool bStartImmediately)
{
	if (!IsMovementAbility(ActivationContext.AbilityTag))
	{
		// Movement Ability가 아닌 경우 들어오는 분기입니다.
		for (const FAbilityActivationContext& PlayerAbilityActivationContext : PlayerAbilityActivationContexts)
		{
			if (PlayerAbilityActivationContext.Index == ActivationContext.Index)
			{
				// 이미 사용 대기 중인 카드인 경우 얼리리턴합니다.
				return;
			}
		}
	}
	PlayerAbilityActivationContexts.Add(MoveTemp(ActivationContext));

	if (bStartImmediately && !IsResolvingPlayerAbility())
	{
		// Ability가 사용 중이 아닐 때만 사용을 시작합니다.
		StartActivatePlayerAbility();
	}
}

void UAbilityResolverComponent::StartActivatePlayerAbility()
{
	if (!PlayerAbilityActivationContexts.IsEmpty())
	{
		CurrentActivatorTeamSide = ETeamSide::Player;

		// Ability 발동 처리 중임을 기록하고, 발동 시도가 끝나면 다시 false로 되돌립니다.
		bIsHandlingAbilityActivation = true;
		const ETryAbilityActivationResult Result = TryActivateNextPlayerAbility();
		bIsHandlingAbilityActivation = false;

		HandlePlayerAbilityActivationResult(Result);
		ProcessPendingAbilityCallbacks();

		return;
	}
	OnFinishActivationQueue.ExecuteIfBound();
}

ETryAbilityActivationResult UAbilityResolverComponent::TryActivateNextPlayerAbility()
{
	if (PlayerAbilityActivationContexts.IsEmpty())
	{
		// 모든 Ability를 사용한 경우 들어오는 분기입니다.
		return ETryAbilityActivationResult::AllAbilityUsed;
	}

	// 아직 사용되지 않은 Ability 중 가장 먼저 사용한 Ability의 사용을 시작합니다.
	FAbilityActivationContext* ActivationContext = &PlayerAbilityActivationContexts[0];
	return TryActivateAbility(ActivationContext);
}

void UAbilityResolverComponent::HandlePlayerAbilityActivationResult(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::Success:
		bIsResolvingPlayerAbility = true;
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivatorTeamSide = ETeamSide::None;
		bIsResolvingPlayerAbility = false;
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 일단 진행은 합니다."));
	case ETryAbilityActivationResult::FailedNotActivated:
	case ETryAbilityActivationResult::EmptyTile:
	default:
		ProcessAllPlayerAbilitiesFailed();
		break;
	}
}

void UAbilityResolverComponent::ProcessAllPlayerAbilitiesFailed()
{
	// 카드 사용 실패 시 모든 카드에 대해 사용 실패를 콜백하고 ActivationContext를 정리합니다.
	for (const FAbilityActivationContext& PlayerAbilityActivationContext : PlayerAbilityActivationContexts)
	{
		if (!IsMovementAbility(PlayerAbilityActivationContext.AbilityTag))
		{
			OnResolveUseCard.ExecuteIfBound(PlayerAbilityActivationContext.Index, false);
		}
	}
	PlayerAbilityActivationContexts.Reset();
	CurrentActivatorTeamSide = ETeamSide::None;
	bIsResolvingPlayerAbility = false;
}

void UAbilityResolverComponent::SetEnemyAbilityActivationContext(TArray<FAbilityActivationContext>&& ActivationContext)
{
	EnemyAbilityActivationContexts = MoveTemp(ActivationContext);
}

void UAbilityResolverComponent::StartActivateEnemyAbility()
{
	while (true)
	{
		CurrentActivatorTeamSide = ETeamSide::Enemy;

		bIsHandlingAbilityActivation = true;
		const ETryAbilityActivationResult Result = TryActivateNextEnemyAbility();
		bIsHandlingAbilityActivation = false;

		HandleEnemyAbilityActivationResult(Result);
		if (ProcessPendingAbilityCallbacks())
		{
			return;
		}

		if (Result == ETryAbilityActivationResult::FailedLogicError)
		{
			continue;
		}

		return;
	}
}

ETryAbilityActivationResult UAbilityResolverComponent::TryActivateNextEnemyAbility()
{
	if (EnemyAbilityActivationContexts.IsEmpty())
	{
		// 모든 Ability를 사용한 경우 들어오는 분기입니다.
		return ETryAbilityActivationResult::AllAbilityUsed;
	}

	// 아직 사용되지 않은 Ability 중 가장 높은 우선순위를 가진 Enemy Ability의 사용을 시작합니다.
	FAbilityActivationContext* ActivationContext = &EnemyAbilityActivationContexts[0];

	const ETryAbilityActivationResult Result = TryActivateAbility(ActivationContext);
	if (ActivationContext && ActivationContext->AbilityOwnerASC.IsValid())
	{
		OnAttemptEnemyAbility.ExecuteIfBound(ActivationContext->AbilityOwnerASC.Get()->GetAvatarActor());
	}
	EnemyAbilityActivationContexts.RemoveAt(0);
	return Result;
}

void UAbilityResolverComponent::HandleEnemyAbilityActivationResult(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::Success:
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivatorTeamSide = ETeamSide::None;
		OnFinishActivationQueue.ExecuteIfBound();
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 일단 진행은 합니다."));
		break;
	case ETryAbilityActivationResult::FailedNotActivated:
	case ETryAbilityActivationResult::FailedNoMoveDestination:
		// 모종의 이유로 Ability 발동에 실패하면 Ended가 호출되지 않으므로 여기서 명시적으로 호출합니다.
		OnAbilityActivationFailed();
		break;
	default:
		ResetEnemyActivationContext();
		break;
	}
}

void UAbilityResolverComponent::ResetEnemyActivationContext()
{
	EnemyAbilityActivationContexts.Reset();
	CurrentActivatorTeamSide = ETeamSide::None;
}

void UAbilityResolverComponent::ActivateAbility(FAbilityActivationContext& ActivationContext, const ETeamSide TeamSide)
{
	CurrentActivatorTeamSide = TeamSide;

	// Queue와 관계 없이 Ability를 즉시 발동하려는 경우 호출되는 함수기 때문에, 반환값에 따른 별도의 처리는 하지 않습니다.
	bIsHandlingAbilityActivation = true;
	const ETryAbilityActivationResult Result = TryActivateAbility(&ActivationContext);
	LETHE_LOG(LogAbilityResolver, Log, "Ability Activate Result: %s", *LogHelper::EnumToString(Result));
	bIsHandlingAbilityActivation = false;
	if (Result == ETryAbilityActivationResult::FailedLogicError)
	{
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다."));
	}
}

ETryAbilityActivationResult UAbilityResolverComponent::TryActivateAbility(FAbilityActivationContext* ActivationContext)
{
	if (!ActivationContext)
	{
		return ETryAbilityActivationResult::FailedLogicError;
	}

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem || !ActivationContext->AbilityOwnerASC.IsValid())
	{
		return ETryAbilityActivationResult::FailedFatal;
	}

	UAbilitySystemComponent* AbilityOwnerASC = ActivationContext->AbilityOwnerASC.Get();
	const AActor* AvatarActor = AbilityOwnerASC->GetAvatarActor();
	if (!AvatarActor)
	{
		return ETryAbilityActivationResult::FailedFatal;
	}
	ActivationContext->Payload.Instigator = AvatarActor;
	LETHE_LOG(LogAbilityResolver, Log, "Ability Instigator: %s", *AvatarActor->GetName());

	if (IsMovementAbility(ActivationContext->AbilityTag))
	{
		UMoveAbilityPayload* MovePayload = NewObject<UMoveAbilityPayload>(this);

		// 모든 경로 생성 함수들은 StartTile을 제외하고 생성하므로, 여기서 직접 추가합니다.
		if (ATile* StartTile = TileManagerSubsystem->GetTileUnderActor(AbilityOwnerASC->GetAvatarActor()))
		{
			MovePayload->PathTiles.Add(StartTile);
		}

		for (const auto& PathTile : ActivationContext->PathTiles)
		{
			if (PathTile.IsValid())
			{
				MovePayload->PathTiles.Add(PathTile.Get());
			}
		}

		// 시작 타일과 목적지 타일까지 해서 최소 2개의 타일이 배열 내에 들어있어야 합니다.
		if (MovePayload->PathTiles.Num() < 2)
		{
			return ETryAbilityActivationResult::FailedNoMoveDestination;
		}

		ActivationContext->Payload.OptionalObject = MovePayload;
	}
	else
	{
		bool bHasCombatTarget = false;
		
		if (CurrentActivatorTeamSide == ETeamSide::Enemy)
		{
			// Enemy가 사용한 경우, 시전 직전에 TargetTiles를 갱신합니다.
			if (const FGameplayAbilitySpec* Spec = ActivationContext->AbilityOwnerASC->FindAbilitySpecFromHandle(ActivationContext->AbilitySpecHandle))
			{
				if (const ULetheCardAbility* CardAbility = Cast<ULetheCardAbility>(Spec->Ability))
				{
					FEffectTargetTileSelectorContext Context;
					Context.AvatarActor = AbilityOwnerASC->GetAvatarActor();
					Context.TargetingIntent = ActivationContext->TargetingIntent;

					CardAbility->GetTargetTiles(Context);

					ActivationContext->TargetSelectionResults = MoveTemp(Context.OutTargetResults);
				}
			}
		}
		
		for (FTargetSelectionResult& TargetResult : ActivationContext->TargetSelectionResults)
		{
			bool bHasCombatTargetForCurrentResult = false;
			for (FSelectedTarget& Target : TargetResult.Targets)
			{
				if (Target.ActorOnTile.IsValid())
				{
					// 유효한 대상이 하나라도 있는 경우 이를 기록합니다.
					bHasCombatTargetForCurrentResult = true;
					bHasCombatTarget = true;
					break;
				}
			}

			if (CurrentActivatorTeamSide == ETeamSide::Enemy && !bHasCombatTargetForCurrentResult)
			{
				// Enemy가 사용한 Ability이고, 이번 Result에 유효한 대상이 하나도 없는 경우, TargetTile 위치에 DummyActor를 놓습니다.
				ATile* DummyTargetTile = nullptr;
				for (const FSelectedTarget& Target : TargetResult.Targets)
				{
					if (Target.TargetTile.IsValid())
					{
						DummyTargetTile = Target.TargetTile.Get();
						break;
					}
				}

				if (DummyActor && DummyTargetTile)
				{
					const FVector DummyActorLocation = DummyTargetTile->GetActorLocation() + FVector(0.f, 0.f, 45.f);
					DummyActor->SetActorLocation(DummyActorLocation);

					FSelectedTarget& DummyTarget = TargetResult.Targets.AddDefaulted_GetRef();
					DummyTarget.TargetTile = DummyTargetTile;
					DummyTarget.ActorOnTile = DummyActor;
				}
			}
		}

		if (CurrentActivatorTeamSide == ETeamSide::Player && !bHasCombatTarget && !ActivationContext->bCanUseOnTile)
		{
			// Player가 사용한 Ability이고, 유효한 대상이 하나도 없으며, Ability가 대상 없이 발동 불가능한 경우 얼리리턴합니다.
			return ETryAbilityActivationResult::EmptyTile;
		}

		FGameplayAbilityTargetData_TargetSelectionResults* TargetSelectionResultsData = new FGameplayAbilityTargetData_TargetSelectionResults();
		TargetSelectionResultsData->TargetSelectionResults = ActivationContext->TargetSelectionResults;
		ActivationContext->Payload.TargetData.Add(TargetSelectionResultsData);
	}

	// ASC를 캐싱하고 콜백을 붙여둡니다.
	CurrentActivatorASC = AbilityOwnerASC;
	OnAbilityEndedDelegate = AbilityOwnerASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);

	const bool bSuccess = AbilityOwnerASC->TriggerAbilityFromGameplayEvent(ActivationContext->AbilitySpecHandle, AbilityOwnerASC->AbilityActorInfo.Get(), ActivationContext->AbilityTag, &ActivationContext->Payload, *AbilityOwnerASC);
	if (!bSuccess)
	{
		if (CurrentActivatorASC.IsValid())
		{
			CurrentActivatorASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
			CurrentActivatorASC.Reset();
		}

		if (CurrentActivatorTeamSide == ETeamSide::Player)
		{
			return ETryAbilityActivationResult::FailedNotActivated;
		}

		// Enemy는 코스트나 마나 등의 개념이 없으며, TargetActor가 없더라도 Ability를 발동하기 때문에 Ability 발동에 실패했다면 로직에 문제가 있는 상황입니다.
		return ETryAbilityActivationResult::FailedLogicError;
	}

	return ETryAbilityActivationResult::Success;
}

bool UAbilityResolverComponent::IsMovementAbility(const FGameplayTag& AbilityTag) const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	return AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move) || AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Swap);
}

void UAbilityResolverComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	LETHE_LOG(LogAbilityResolver, Log, "Ability Ended");
	// Ability를 성공적으로 발동해 EndAbility까지 호출됐을 때 콜백을 통해 이곳으로 들어옵니다.
	// 턴제 게임인 프로젝트 특성상 Ability 발동 도중 Cancel되는 경우는 존재하지 않습니다.
	if (CurrentActivatorASC.IsValid())
	{
		CurrentActivatorASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
		CurrentActivatorASC.Reset();
	}

	if (bIsHandlingAbilityActivation)
	{
		// 애니메이션이 없는 즉발성 Ability는 EndAbility가 바로 호출되어 들어오는 분기로, 성공 처리를 보류합니다.
		bPendingAbilitySucceeded = true;
		return;
	}

	// 보류할 필요가 없는 경우 즉시 성공 처리를 진행합니다.
	ProcessAbilitySucceeded();
}

bool UAbilityResolverComponent::ProcessPendingAbilityCallbacks()
{
	if (bPendingAbilityFailed)
	{
		// 보류했던 실패 처리를 수행합니다.
		bPendingAbilityFailed = false;
		bPendingAbilitySucceeded = false;
		ProcessAbilityFailed();
		return true;
	}

	if (bPendingAbilitySucceeded)
	{
		// 보류했던 성공 처리를 수행합니다.
		bPendingAbilitySucceeded = false;
		ProcessAbilitySucceeded();
		return true;
	}

	return false;
}

void UAbilityResolverComponent::ProcessAbilitySucceeded()
{
	// Player와 Enemy에 따라 필요한 처리를 하고 다음 Ability 발동을 시도합니다.
	switch (CurrentActivatorTeamSide)
	{
	case ETeamSide::Player:
		{
			if (PlayerAbilityActivationContexts.IsValidIndex(0))
			{
				if (!IsMovementAbility(PlayerAbilityActivationContexts[0].AbilityTag))
				{
					OnResolveUseCard.ExecuteIfBound(PlayerAbilityActivationContexts[0].Index, true);
				}
				PlayerAbilityActivationContexts.RemoveAt(0, EAllowShrinking::No);
			}
			bIsResolvingPlayerAbility = !PlayerAbilityActivationContexts.IsEmpty();
			StartActivatePlayerAbility();
		}
		break;
	case ETeamSide::Enemy:
		StartActivateEnemyAbility();
		break;
	default:
		break;
	}
}

void UAbilityResolverComponent::OnAbilityActivationFailed()
{
	// Ability를 발동했으나, 내부 로직에 의해 실패한 경우(코스트 부족 등) 이곳으로 들어옵니다.
	if (CurrentActivatorASC.IsValid())
	{
		CurrentActivatorASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
		CurrentActivatorASC.Reset();
	}

	if (bIsHandlingAbilityActivation)
	{
		// Ability 내부 로직이 발동 실패를 알린 경우 들어오는 분기로, 실패 처리를 보류합니다.
		bPendingAbilityFailed = true;
		return;
	}

	ProcessAbilityFailed();
}

void UAbilityResolverComponent::ProcessAbilityFailed()
{
	switch (CurrentActivatorTeamSide)
	{
	case ETeamSide::Player:
		// 등록되어 있던 모든 Ability 발동을 취소합니다.
		ProcessAllPlayerAbilitiesFailed();
		bIsResolvingPlayerAbility = false;
		break;
	case ETeamSide::Enemy:
		// Enemy의 경우 정상적으로 수행됐다면 이 함수로 들어올 일이 없지만, 일단 다음 Ability 발동을 진행합니다.
		StartActivateEnemyAbility();
		break;
	default:
		break;
	}
}

bool UAbilityResolverComponent::IsResolvingPlayerAbility() const
{
	return bIsResolvingPlayerAbility;
}
