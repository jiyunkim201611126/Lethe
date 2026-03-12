// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityResolverComponent.h"

#include "AbilitySystemComponent.h"
#include "LetheGameState.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

UAbilityResolverComponent::UAbilityResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PlayerAbilityActivationData.Reserve(MAX_HAND_COUNT);
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
		CurrentActivationCharacterTeamSide = ETeamSide::Player;
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivationCharacterTeamSide = ETeamSide::None;
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 김지윤한테 문의 바랍니다. 일단 진행은 합니다."));
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

void UAbilityResolverComponent::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	EnemyAbilityActivationPriorities.HeapPush(ActivationData.Index, TLess<int32>());
	EnemyAbilityActivationData.Emplace(ActivationData.Index, ActivationData);
}

void UAbilityResolverComponent::SetTargetTileForEnemy(const int32 Priority, ATile* TargetTile)
{
	if (FAbilityActivationData* Data = EnemyAbilityActivationData.Find(Priority))
	{
		Data->TargetTile = TargetTile;
	}
}

void UAbilityResolverComponent::StartActivateEnemyAbility()
{
	while (true)
	{
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
	if (EnemyAbilityActivationPriorities.IsEmpty())
	{
		// 모든 Ability를 사용한 경우 들어오는 분기입니다.
		return ETryAbilityActivationResult::AllAbilityUsed;
	}

	// 아직 사용되지 않은 Ability 중 가장 높은 우선순위를 가진 Enemy Ability의 사용을 시작합니다.
	int32 CurrentPriority;
	EnemyAbilityActivationPriorities.HeapPop(CurrentPriority, TLess<int32>());
	FAbilityActivationData* ActivationData = EnemyAbilityActivationData.Find(CurrentPriority);
	
	const ETryAbilityActivationResult Result = TryActivateAbility(ActivationData);
	if (ActivationData && ActivationData->AbilityOwnerASC.IsValid() && OnEnemyAbilityActivatedDelegate.IsBound())
	{
		OnEnemyAbilityActivatedDelegate.Broadcast(ActivationData->AbilityOwnerASC.Get()->GetAvatarActor());
	}
	EnemyAbilityActivationData.Remove(CurrentPriority);
	return Result;
}

void UAbilityResolverComponent::HandleEnemyAbilityActivationResult(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::Success:
		CurrentActivationCharacterTeamSide = ETeamSide::Enemy;
		break;
	case ETryAbilityActivationResult::AllAbilityUsed:
		// 모든 Ability 사용을 마쳤다면 DrawPhase로 돌아갑니다.
		if (ALetheGameState* LetheGameState = GetOwner<ALetheGameState>())
		{
			LetheGameState->GoDrawPhase();
		}
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 김지윤한테 문의 바랍니다. 일단 진행은 합니다."));
		break;
	case ETryAbilityActivationResult::FailedNotActivated:
	case ETryAbilityActivationResult::FailedNoneTargetTileToMove:
		// 모종의 이유로 Ability 발동에 실패하면 Ended가 호출되지 않으므로 여기서 명시적으로 호출합니다.
		OnAbilityEnded(false);
		break;
	default:
		ResetEnemyActivationData();
		break;
	}
}

void UAbilityResolverComponent::ResetEnemyActivationData()
{
	EnemyAbilityActivationPriorities.Reset();
	EnemyAbilityActivationData.Reset();
	CurrentActivationCharacterTeamSide = ETeamSide::None;
}

ETryAbilityActivationResult UAbilityResolverComponent::TryActivateAbility(FAbilityActivationData* ActivationData) const
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
		
		AActor* Target = TileManagerSubsystem->GetActorOnTile(ActivationData->TargetTile.Get());
		ActivationData->Payload.Target = Target;
	}
	
	const bool bSuccess = AbilityOwnerASC->TriggerAbilityFromGameplayEvent(ActivationData->AbilitySpecHandle, AbilityOwnerASC->AbilityActorInfo.Get(), ActivationData->AbilityTag, &ActivationData->Payload, *AbilityOwnerASC);
	if (!bSuccess)
	{
		switch (CurrentActivationCharacterTeamSide)
		{
		case ETeamSide::Player:
			return ETryAbilityActivationResult::FailedNotActivated;
		default:
			return ETryAbilityActivationResult::FailedLogicError;
		}
	}

	return ETryAbilityActivationResult::Success;
}

void UAbilityResolverComponent::OnAbilityEnded(const bool bSuccess)
{
	switch (CurrentActivationCharacterTeamSide)
	{
	case ETeamSide::Player:
		{
			if (bSuccess)
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
			}
			else
			{
				ProcessAllPlayerAbilitiesFailed();
			}
			
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

bool UAbilityResolverComponent::IsActivatingPlayerAbility() const
{
	return bIsActivatingPlayerAbility;
}
