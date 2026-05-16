// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityResolverComponent.h"

#include "AbilitySystemComponent.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

UAbilityResolverComponent::UAbilityResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PlayerAbilityActivationData.Reserve(MAX_HAND_COUNT);
}

void UAbilityResolverComponent::SetDummyActor(AActor* InDummyActor)
{
	DummyActor = InDummyActor;
}

void UAbilityResolverComponent::EnqueuePlayerAbilityActivationData(const FAbilityActivationData& ActivationData, const bool bStartImmediately)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const bool bIsMovementAbility = ActivationData.AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move) || ActivationData.AbilityTag.MatchesTag(LetheGameplayTags.Ability_Swap);
	if (!bIsMovementAbility)
	{
		// Movement Ability가 아닌 경우 들어오는 분기입니다.
		for (const FAbilityActivationData& RegisteredActivationData : PlayerAbilityActivationData)
		{
			if (RegisteredActivationData.Index == ActivationData.Index)
			{
				// 이미 사용 대기 중인 카드인 경우 얼리리턴합니다.
				return;
			}
		}
	}
	PlayerAbilityActivationData.Add(ActivationData);

	if (bStartImmediately && !IsResolvingPlayerAbility())
	{
		// Ability가 사용 중이 아닐 때만 사용을 시작합니다.
		StartActivatePlayerAbility();
	}
}

void UAbilityResolverComponent::StartActivatePlayerAbility()
{
	while (!PlayerAbilityActivationData.IsEmpty())
	{
		CurrentActivationCharacterTeamSide = ETeamSide::Player;

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
	if (PlayerAbilityActivationData.IsEmpty())
	{
		// 모든 Ability를 사용한 경우 들어오는 분기입니다.
		return ETryAbilityActivationResult::AllAbilityUsed;
	}

	// 아직 사용되지 않은 Ability 중 가장 먼저 사용한 Ability의 사용을 시작합니다.
	FAbilityActivationData* ActivationData = &PlayerAbilityActivationData[0];
	if (!ActivationData)
	{
		return ETryAbilityActivationResult::FailedLogicError;
	}
	
	return TryActivateAbility(ActivationData);
}

void UAbilityResolverComponent::HandlePlayerAbilityActivationResult(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::Success:
		bIsResolvingPlayerAbility = true;
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivationCharacterTeamSide = ETeamSide::None;
		bIsResolvingPlayerAbility = false;
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 일단 진행은 합니다."));
	case ETryAbilityActivationResult::FailedNotActivated:
	default:
		ProcessAllPlayerAbilitiesFailed();
		break;
	}
}

void UAbilityResolverComponent::ProcessAllPlayerAbilitiesFailed()
{
	// 카드 사용 실패 시 모든 카드에 대해 사용 실패를 콜백하고 ActivationData를 정리합니다.
	for (const FAbilityActivationData& WaitingCardData : PlayerAbilityActivationData)
	{
		OnResolveUseCard.ExecuteIfBound(WaitingCardData.Index, false);
	}
	PlayerAbilityActivationData.Reset();
	CurrentActivationCharacterTeamSide = ETeamSide::None;
	bIsResolvingPlayerAbility = false;
}

void UAbilityResolverComponent::SetEnemyAbilityActivationData(TArray<FAbilityActivationData>&& ActivationData)
{
	EnemyAbilityActivationData = MoveTemp(ActivationData);
}

void UAbilityResolverComponent::StartActivateEnemyAbility()
{
	while (true)
	{
		CurrentActivationCharacterTeamSide = ETeamSide::Enemy;
		
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
	if (EnemyAbilityActivationData.IsEmpty())
	{
		// 모든 Ability를 사용한 경우 들어오는 분기입니다.
		return ETryAbilityActivationResult::AllAbilityUsed;
	}

	// 아직 사용되지 않은 Ability 중 가장 높은 우선순위를 가진 Enemy Ability의 사용을 시작합니다.
	FAbilityActivationData* ActivationData = &EnemyAbilityActivationData[0];
	
	const ETryAbilityActivationResult Result = TryActivateAbility(ActivationData);
	if (ActivationData && ActivationData->AbilityOwnerASC.IsValid())
	{
		OnActivateEnemyAbility.ExecuteIfBound(ActivationData->AbilityOwnerASC.Get()->GetAvatarActor());
	}
	EnemyAbilityActivationData.RemoveAt(0);
	return Result;
}

void UAbilityResolverComponent::HandleEnemyAbilityActivationResult(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::Success:
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivationCharacterTeamSide = ETeamSide::None;
		OnFinishActivationQueue.ExecuteIfBound();
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 일단 진행은 합니다."));
		break;
	case ETryAbilityActivationResult::FailedNotActivated:
	case ETryAbilityActivationResult::FailedNoneTargetTileToMove:
		// 모종의 이유로 Ability 발동에 실패하면 Ended가 호출되지 않으므로 여기서 명시적으로 호출합니다.
		OnAbilityActivationFailed();
		break;
	default:
		ResetEnemyActivationData();
		break;
	}
}

void UAbilityResolverComponent::ResetEnemyActivationData()
{
	EnemyAbilityActivationData.Reset();
	CurrentActivationCharacterTeamSide = ETeamSide::None;
}

void UAbilityResolverComponent::ActivateAbility(FAbilityActivationData& ActivationData, const ETeamSide TeamSide)
{
	CurrentActivationCharacterTeamSide = TeamSide;
	
	// Queue와 관계 없이 Ability를 즉시 발동하려는 경우 호출되는 함수기 때문에, 반환값에 따른 별도의 처리는 하지 않습니다.
	bIsHandlingAbilityActivation = true;
	const ETryAbilityActivationResult Result = TryActivateAbility(&ActivationData);
	LETHE_LOG(LogAbilityResolver, Log, "Ability Activate Result: %s", *LogHelper::EnumToString(Result));
	bIsHandlingAbilityActivation = false;
	if (Result == ETryAbilityActivationResult::FailedLogicError)
	{
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다."));
	}
}

ETryAbilityActivationResult UAbilityResolverComponent::TryActivateAbility(FAbilityActivationData* ActivationData)
{
	LETHE_LOG(LogAbilityResolver, Log, "Ability Instigator: %s", *ActivationData->AbilityOwnerASC->GetAvatarActor()->GetName());
	if (!ActivationData)
	{
		return ETryAbilityActivationResult::FailedLogicError;
	}
	
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem || !ActivationData->AbilityOwnerASC.IsValid())
	{
		return ETryAbilityActivationResult::FailedFatal;
	}

	UAbilitySystemComponent* AbilityOwnerASC = ActivationData->AbilityOwnerASC.Get();
	ActivationData->Payload.Instigator = AbilityOwnerASC->GetAvatarActor();
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const bool bIsMovementAbility = ActivationData->AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move) || ActivationData->AbilityTag.MatchesTag(LetheGameplayTags.Ability_Swap);
	if (bIsMovementAbility)
	{
		UMoveAbilityPayload* MovePayload = NewObject<UMoveAbilityPayload>(this);

		/** 모든 경로 생성 함수들은 StartTile을 제외하고 생성하므로, 여기서 직접 추가합니다. */
		if (ATile* StartTile = TileManagerSubsystem->GetTileUnderActor(AbilityOwnerASC->GetAvatarActor()))
		{
			MovePayload->PathTiles.Add(StartTile);
		}
		
		for (const auto& TargetTile : ActivationData->TargetTiles)
		{
			if (TargetTile.IsValid())
			{
				MovePayload->PathTiles.Add(TargetTile.Get());
			}
		}
		
		if (MovePayload->PathTiles.IsEmpty())
		{
			return ETryAbilityActivationResult::FailedNoneTargetTileToMove;
		}
		
		ActivationData->Payload.OptionalObject = MovePayload;
	}
	else
	{
		if (ActivationData->TargetTiles.IsEmpty())
		{
			return ETryAbilityActivationResult::FailedLogicError;
		}

		TArray<TWeakObjectPtr<AActor>> TargetActors;
		for (const auto& TargetTile : ActivationData->TargetTiles)
		{
			if (TargetTile.IsValid())
			{
				TargetActors.Add(TileManagerSubsystem->GetActorOnTile(TargetTile.Get()));
			}
		}

		FGameplayAbilityTargetData_ActorArray* ActorArrayData = new FGameplayAbilityTargetData_ActorArray();
		if (TargetActors.IsEmpty())
		{
			switch (CurrentActivationCharacterTeamSide)
			{
			case ETeamSide::Player:
				// 플레이어는 Tile 위에 대상이 없는 상태로 여기까지 왔다면 로직 오류입니다.
				return ETryAbilityActivationResult::FailedLogicError;
			case ETeamSide::Enemy:
				{
					// 적의 경우, 타일 위에 캐릭터가 없다면 DummyActor를 그 위치에 올려두고 Ability를 발동합니다.
					const FVector DummyActorLocation = ActivationData->TargetTiles[0].Get()->GetActorLocation() + FVector(0.f, 0.f, 45.f);
					DummyActor->SetActorLocation(DummyActorLocation);
					TargetActors.Add(DummyActor);
				}
				break;
			default:
				break;
			}
		}

		ActorArrayData->SetActors(TargetActors);
		ActivationData->Payload.OptionalObject = ActivationData->TargetTiles[0].Get();
		ActivationData->Payload.TargetData.Add(ActorArrayData);
	}
	
	// ASC를 캐싱하고 콜백을 붙여둡니다.
	CurrentActivationASC = AbilityOwnerASC;
	OnAbilityEndedDelegate = AbilityOwnerASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);
	
	const bool bSuccess = AbilityOwnerASC->TriggerAbilityFromGameplayEvent(ActivationData->AbilitySpecHandle, AbilityOwnerASC->AbilityActorInfo.Get(), ActivationData->AbilityTag, &ActivationData->Payload, *AbilityOwnerASC);
	if (!bSuccess)
	{
		if (CurrentActivationASC.IsValid())
		{
			CurrentActivationASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
			CurrentActivationASC.Reset();
		}
		
		if (CurrentActivationCharacterTeamSide == ETeamSide::Player)
		{
			return ETryAbilityActivationResult::FailedNotActivated;
		}
		
		// Enemy는 코스트나 마나 등의 개념이 없으며, TargetActor가 없더라도 Ability를 발동하기 때문에 Ability 발동에 실패했다면 로직에 문제가 있는 상황입니다.
		return ETryAbilityActivationResult::FailedLogicError;
	}

	return ETryAbilityActivationResult::Success;
}

void UAbilityResolverComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	LETHE_LOG(LogAbilityResolver, Log, "Ability Ended");
	// Ability를 성공적으로 발동해 EndAbility까지 호출됐을 때 콜백을 통해 이곳으로 들어옵니다.
	// 턴제 게임인 프로젝트 특성상 Ability 발동 도중 Cancel되는 경우는 존재하지 않습니다.
	if (CurrentActivationASC.IsValid())
	{
		CurrentActivationASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
		CurrentActivationASC.Reset();
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
	switch (CurrentActivationCharacterTeamSide)
	{
	case ETeamSide::Player:
		{
			if (PlayerAbilityActivationData.IsValidIndex(0))
			{
				const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
				const bool bIsMovementAbility = PlayerAbilityActivationData[0].AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move) || PlayerAbilityActivationData[0].AbilityTag.MatchesTag(LetheGameplayTags.Ability_Swap);
				if (!bIsMovementAbility)
				{
					OnResolveUseCard.ExecuteIfBound(PlayerAbilityActivationData[0].Index, true);
				}
				PlayerAbilityActivationData.RemoveAt(0, EAllowShrinking::No);
			}
			bIsResolvingPlayerAbility = !PlayerAbilityActivationData.IsEmpty();
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
	// Ability를 발동했으나, 내부 로직에 의해 실패한 경우(층 수 차이, 코스트 부족 등) 이곳으로 들어옵니다.
	if (CurrentActivationASC.IsValid())
	{
		CurrentActivationASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
		CurrentActivationASC.Reset();
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
	switch (CurrentActivationCharacterTeamSide)
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
