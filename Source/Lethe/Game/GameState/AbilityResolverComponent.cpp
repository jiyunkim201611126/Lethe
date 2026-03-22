// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityResolverComponent.h"

#include "AbilitySystemComponent.h"
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

void UAbilityResolverComponent::AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (!ActivationData.AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move))
	{
		// Move Ability가 아닌 경우 들어오는 분기입니다.
		for (const FAbilityActivationData& RegisteredActivationData : PlayerAbilityActivationData)
		{
			if (RegisteredActivationData.Index == ActivationData.Index)
			{
				// 이미 사용 대기 중인 카드인 경우 얼리리턴합니다.
				return;
			}
		}
	}

	// 대기열에 추가한 후 Ability 사용을 시작합니다.
	PlayerAbilityActivationData.Emplace(ActivationData);
	if (!IsActivatingPlayerAbility())
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
		const ETryAbilityActivationResult Result = TryActivateNextPlayerAbility();
		HandlePlayerAbilityActivationResult(Result);
		
		if (Result == ETryAbilityActivationResult::FailedLogicError)
		{
			continue;
		}
		
		return;
	}
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
		bIsActivatingPlayerAbility = true;
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivationCharacterTeamSide = ETeamSide::None;
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 일단 진행은 합니다."));
		break;
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
		OnUseCardResolved.ExecuteIfBound(WaitingCardData.Index, false);
	}
	PlayerAbilityActivationData.Reset();
	CurrentActivationCharacterTeamSide = ETeamSide::None;
	bIsActivatingPlayerAbility = false;
}

void UAbilityResolverComponent::ActivateEnemyAbility(FAbilityActivationData& ActivationData)
{
	// Queue와 관계 없이 Ability를 즉시 발동하려는 경우 호출되는 함수기 때문에, 반환값에 따른 별도의 처리는 하지 않습니다.
	const ETryAbilityActivationResult Result = TryActivateAbility(&ActivationData);
	if (Result == ETryAbilityActivationResult::FailedLogicError)
	{
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다."));
	}
}

void UAbilityResolverComponent::SetEnemyAbilityActivationData(TArray<FAbilityActivationData>&& ActivationData)
{
	EnemyAbilityActivationData = MoveTemp(ActivationData);
}

void UAbilityResolverComponent::SortEnemyAbilityActivationData()
{
	EnemyAbilityActivationData.Sort([](const FAbilityActivationData& A, const FAbilityActivationData& B)
	{
		return A.Index < B.Index;
	});
}

void UAbilityResolverComponent::StartActivateEnemyAbility()
{
	while (true)
	{
		CurrentActivationCharacterTeamSide = ETeamSide::Enemy;
		const ETryAbilityActivationResult Result = TryActivateNextEnemyAbility();
		HandleEnemyAbilityActivationResult(Result);

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
	if (ActivationData && ActivationData->AbilityOwnerASC.IsValid() && OnEnemyAbilityActivated.IsBound())
	{
		OnEnemyAbilityActivated.Broadcast(ActivationData->AbilityOwnerASC.Get()->GetAvatarActor());
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
		OnEnemyActivationQueueFinished.ExecuteIfBound();
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

ETryAbilityActivationResult UAbilityResolverComponent::TryActivateAbility(FAbilityActivationData* ActivationData)
{
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
	if (ActivationData->AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move))
	{
		if (!ActivationData->TargetTile.IsValid())
		{
			return ETryAbilityActivationResult::FailedNoneTargetTileToMove;
		}
		
		ActivationData->Payload.OptionalObject = ActivationData->TargetTile.Get();
	}
	else
	{
		if (!ActivationData->TargetTile.IsValid())
		{
			return ETryAbilityActivationResult::FailedLogicError;
		}
		
		if (AActor* Target = TileManagerSubsystem->GetActorOnTile(ActivationData->TargetTile.Get()))
		{
			ActivationData->Payload.Target = Target;
		}
		else
		{
			if (CurrentActivationCharacterTeamSide == ETeamSide::Player)
			{
				// 플레이어는 Tile 위에 대상이 없는 상태로 여기까지 왔다면 로직 오류입니다.
				return ETryAbilityActivationResult::FailedLogicError;
			}
			
			// 적의 경우, 타일 위에 캐릭터가 없다면 DummyActor를 그 위치에 올려두고 Ability를 발동합니다.
			const FVector DummyActorLocation = ActivationData->TargetTile.Get()->GetActorLocation() + FVector(0.f, 0.f, 45.f);
			DummyActor->SetActorLocation(DummyActorLocation);
			ActivationData->Payload.Target = DummyActor;
		}
	}
	
	const bool bSuccess = AbilityOwnerASC->TriggerAbilityFromGameplayEvent(ActivationData->AbilitySpecHandle, AbilityOwnerASC->AbilityActorInfo.Get(), ActivationData->AbilityTag, &ActivationData->Payload, *AbilityOwnerASC);
	if (!bSuccess)
	{
		if (CurrentActivationCharacterTeamSide == ETeamSide::Player)
		{
			return ETryAbilityActivationResult::FailedNotActivated;
		}
		
		// Enemy는 코스트나 마나 등의 개념이 없으며, TargetActor가 없더라도 Ability를 발동하기 때문에 Ability 발동에 실패했다면 로직에 문제가 있는 상황입니다.
		return ETryAbilityActivationResult::FailedLogicError;
	}

	// 성공적으로 Ability를 발동한 경우 ASC를 캐싱하고 콜백을 붙여둡니다.
	CurrentActivationASC = AbilityOwnerASC;
	OnAbilityEndedDelegate = AbilityOwnerASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);

	return ETryAbilityActivationResult::Success;
}

void UAbilityResolverComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (CurrentActivationASC.IsValid())
	{
		CurrentActivationASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
		CurrentActivationASC.Reset();
	}
	
	switch (CurrentActivationCharacterTeamSide)
	{
	case ETeamSide::Player:
		{
			if (PlayerAbilityActivationData.IsValidIndex(0))
			{
				const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
				if (!PlayerAbilityActivationData[0].AbilityTag.MatchesTagExact(LetheGameplayTags.Ability_Move))
				{
					OnUseCardResolved.ExecuteIfBound(PlayerAbilityActivationData[0].Index, true);
				}
				PlayerAbilityActivationData.RemoveAt(0, EAllowShrinking::No);
			}
			StartActivatePlayerAbility();
			
			bIsActivatingPlayerAbility = !PlayerAbilityActivationData.IsEmpty();
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
	if (CurrentActivationASC.IsValid())
	{
		CurrentActivationASC->OnAbilityEnded.Remove(OnAbilityEndedDelegate);
		CurrentActivationASC.Reset();
	}
	
	switch (CurrentActivationCharacterTeamSide)
	{
	case ETeamSide::Player:
		ProcessAllPlayerAbilitiesFailed();
		bIsActivatingPlayerAbility = false;
		break;
	case ETeamSide::Enemy:
		StartActivateEnemyAbility();
		break;
	default:
		break;
	}
}

bool UAbilityResolverComponent::IsActivatingPlayerAbility() const
{
	return bIsActivatingPlayerAbility;
}
