// Copyright JETBLU, Inc. All Rights Reserved.

#include "AbilityExecutionManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

UAbilityExecutionManagerComponent::UAbilityExecutionManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAbilityExecutionManagerComponent::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData)
{
	EnemyAbilityActivationPriorities.HeapPush(ActivationData.Index, TLess<int32>());
	EnemyAbilityActivationData.Emplace(ActivationData.Index, ActivationData);
}

void UAbilityExecutionManagerComponent::SetTargetTile(const int32 Priority, ATile* TargetTile)
{
	if (FAbilityActivationData* Data = EnemyAbilityActivationData.Find(Priority))
	{
		Data->TargetTile = TargetTile;
	}
}

void UAbilityExecutionManagerComponent::StartUseAbility()
{
	while (true)
	{
		const EAbilityExecutionResult Result = TryUseNextAbility();
		if (Result == EAbilityExecutionResult::FailedLogicError)
		{
			ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 김지윤한테 문의 바랍니다. 일단 진행은 합니다."));
			continue;
		}
		
		return;
	}
}

EAbilityExecutionResult UAbilityExecutionManagerComponent::TryUseNextAbility()
{
	if (EnemyAbilityActivationPriorities.IsEmpty() || EnemyAbilityActivationData.IsEmpty())
	{
		EnemyAbilityActivationPriorities.Reset();
		EnemyAbilityActivationData.Reset();
		return EAbilityExecutionResult::AllAbilityUsed;
	}
	
	int32 CurrentPriority;
	EnemyAbilityActivationPriorities.HeapPop(CurrentPriority, TLess<int32>());

	FAbilityActivationData* Data = EnemyAbilityActivationData.Find(CurrentPriority);
	if (!Data)
	{
		return EAbilityExecutionResult::FailedLogicError;
	}

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem || !Data->AbilityOwnerASC.IsValid() || !Data->TargetTile.IsValid())
	{
		return EAbilityExecutionResult::FailedFatal;
	}
	
	CurrentActivationASC = Data->AbilityOwnerASC;
	AActor* Target = TileManagerSubsystem->GetActorOnTile(Data->TargetTile.Get());
	Data->Payload.Instigator = CurrentActivationASC->GetAvatarActor();
	Data->Payload.Target = Target;
	
	CurrentActivationASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);
	const bool bSuccess = CurrentActivationASC->TriggerAbilityFromGameplayEvent(Data->AbilitySpecHandle, CurrentActivationASC->AbilityActorInfo.Get(), Data->AbilityTag, &Data->Payload, *CurrentActivationASC);
	if (!bSuccess)
	{
		return EAbilityExecutionResult::FailedLogicError;
	}

	return EAbilityExecutionResult::Success;
}

void UAbilityExecutionManagerComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (CurrentActivationASC.IsValid())
	{
		CurrentActivationASC->OnAbilityEnded.RemoveAll(this);
		CurrentActivationASC.Reset();
	}
	
	StartUseAbility();
}
