// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityResolverComponent.h"

#include "AbilitySystemComponent.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

UAbilityResolverComponent::UAbilityResolverComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	PlayerAbilityActivationData.Reserve(MAX_HAND_COUNT);
}

void UAbilityResolverComponent::AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	for (const FAbilityActivationData& RegisteredActivationData : PlayerAbilityActivationData)
	{
		if (RegisteredActivationData.Index == ActivationData.Index)
		{
			// 이미 사용 대기 중인 카드인 경우 얼리리턴합니다.
			return;
		}
	}
	
	PlayerAbilityActivationData.Emplace(ActivationData);
	if (!bIsProgressingPlayerAbility)
	{
		StartUsePlayerAbility();
	}
}

void UAbilityResolverComponent::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	EnemyAbilityActivationPriorities.HeapPush(ActivationData.Index, TLess<int32>());
	EnemyAbilityActivationData.Emplace(ActivationData.Index, ActivationData);
}

void UAbilityResolverComponent::SetTargetTile(const int32 Priority, ATile* TargetTile)
{
	if (FAbilityActivationData* Data = EnemyAbilityActivationData.Find(Priority))
	{
		Data->TargetTile = TargetTile;
	}
}

void UAbilityResolverComponent::StartUsePlayerAbility()
{
	while (!PlayerAbilityActivationData.IsEmpty())
	{
		const ETryAbilityActivationResult Result = TryUseNextPlayerAbility();
		OnPlayerAbilityUsed(Result);
		
		if (Result == ETryAbilityActivationResult::FailedLogicError)
		{
			ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 김지윤한테 문의 바랍니다. 일단 진행은 합니다."));
			continue;
		}
		
		return;
	}
}

void UAbilityResolverComponent::OnPlayerAbilityUsed(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::AllAbilityUsed:
		CurrentActivationCharacterTeamSide = ETeamSide::None;
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ProcessAllPlayerAbilitiesFailed();
		break;
	case ETryAbilityActivationResult::FailedFatal:
		ProcessAllPlayerAbilitiesFailed();
		break;
	case ETryAbilityActivationResult::Success:
		bIsProgressingPlayerAbility = true;
		CurrentActivationCharacterTeamSide = ETeamSide::Player;
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
	bIsProgressingPlayerAbility = false;
}

void UAbilityResolverComponent::StartUseEnemyAbility()
{
	while (!EnemyAbilityActivationData.IsEmpty())
	{
		const ETryAbilityActivationResult Result = TryUseNextEnemyAbility();
		OnEnemyAbilityUsed(Result);
		
		if (Result == ETryAbilityActivationResult::FailedLogicError)
		{
			ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 김지윤한테 문의 바랍니다. 일단 진행은 합니다."));
			continue;
		}
		
		return;
	}
}

void UAbilityResolverComponent::OnEnemyAbilityUsed(const ETryAbilityActivationResult Result)
{
	switch (Result)
	{
	case ETryAbilityActivationResult::AllAbilityUsed:
		ResetEnemyData();
		break;
	case ETryAbilityActivationResult::FailedLogicError:
		ResetEnemyData();
		break;
	case ETryAbilityActivationResult::FailedFatal:
		ResetEnemyData();
		break;
	case ETryAbilityActivationResult::Success:
		CurrentActivationCharacterTeamSide = ETeamSide::Enemy;
		break;
	}
}

void UAbilityResolverComponent::ResetEnemyData()
{
	EnemyAbilityActivationPriorities.Reset();
	EnemyAbilityActivationData.Reset();
	CurrentActivationCharacterTeamSide = ETeamSide::None;
}

ETryAbilityActivationResult UAbilityResolverComponent::TryUseNextPlayerAbility()
{
	if (PlayerAbilityActivationData.IsEmpty())
	{
		return ETryAbilityActivationResult::AllAbilityUsed;
	}

	FAbilityActivationData* ActivationData = &PlayerAbilityActivationData[0];
	if (!ActivationData)
	{
		return ETryAbilityActivationResult::FailedLogicError;
	}
	
	const ETryAbilityActivationResult Result = TryUseAbility(ActivationData);
	return Result;
}

ETryAbilityActivationResult UAbilityResolverComponent::TryUseNextEnemyAbility()
{
	if (EnemyAbilityActivationPriorities.IsEmpty() || EnemyAbilityActivationData.IsEmpty())
	{
		return ETryAbilityActivationResult::AllAbilityUsed;
	}
	
	int32 CurrentPriority;
	EnemyAbilityActivationPriorities.HeapPop(CurrentPriority, TLess<int32>());

	FAbilityActivationData* ActivationData = EnemyAbilityActivationData.Find(CurrentPriority);
	if (!ActivationData)
	{
		return ETryAbilityActivationResult::FailedLogicError;
	}

	return TryUseAbility(ActivationData);
}

ETryAbilityActivationResult UAbilityResolverComponent::TryUseAbility(FAbilityActivationData* ActivationData) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem || !ActivationData->AbilityOwnerASC.IsValid() || !ActivationData->TargetTile.IsValid())
	{
		return ETryAbilityActivationResult::FailedFatal;
	}
	
	UAbilitySystemComponent* AbilityOwnerASC = ActivationData->AbilityOwnerASC.Get();
	AActor* Target = TileManagerSubsystem->GetActorOnTile(ActivationData->TargetTile.Get());
	ActivationData->Payload.Instigator = AbilityOwnerASC->GetAvatarActor();
	ActivationData->Payload.Target = Target;
	
	const bool bSuccess = AbilityOwnerASC->TriggerAbilityFromGameplayEvent(ActivationData->AbilitySpecHandle, AbilityOwnerASC->AbilityActorInfo.Get(), ActivationData->AbilityTag, &ActivationData->Payload, *AbilityOwnerASC);
	if (!bSuccess)
	{
		return ETryAbilityActivationResult::FailedLogicError;
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
					OnUseCardResolved.ExecuteIfBound(PlayerAbilityActivationData[0].Index, true);
					PlayerAbilityActivationData.RemoveAt(0);
				}
				StartUsePlayerAbility();
			}
			else
			{
				ProcessAllPlayerAbilitiesFailed();
			}
			
			bIsProgressingPlayerAbility = !PlayerAbilityActivationData.IsEmpty();
		}
		break;
	case ETeamSide::Enemy:
		StartUseEnemyAbility();
		break;
	default:
		break;
	}
}

bool UAbilityResolverComponent::IsProgressingPlayerAbility() const
{
	return bIsProgressingPlayerAbility;
}
