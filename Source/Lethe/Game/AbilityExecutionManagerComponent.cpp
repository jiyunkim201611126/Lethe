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
	Priorities.Emplace(ActivationData.Index);
	EnemyAbilityActivationData.Emplace(ActivationData.Index, ActivationData);
}

void UAbilityExecutionManagerComponent::SetTargetTile(const int32 Priority, ATile* TargetTile)
{
	if (FAbilityActivationData* Data = EnemyAbilityActivationData.Find(Priority))
	{
		Data->TargetTile = TargetTile;
	}
}

void UAbilityExecutionManagerComponent::OnEnemyTurnPhaseStarted()
{
	if (Priorities.IsEmpty() || EnemyAbilityActivationData.IsEmpty())
	{
		return;
	}
	
	CurrentPriorityIndex = 0;
	Priorities.Sort();
	TryUseNextAbility();
}

void UAbilityExecutionManagerComponent::TryUseNextAbility()
{
	if (!Priorities.IsValidIndex(CurrentPriorityIndex))
	{
		CurrentPriorityIndex = 0;
		Priorities.Reset();
		EnemyAbilityActivationData.Reset();
		return;
	}

	FAbilityActivationData* Data = EnemyAbilityActivationData.Find(Priorities[CurrentPriorityIndex++]);
	if (!Data || !Data->AbilityOwnerASC.IsValid() || !Data->TargetTile.IsValid())
	{
		TryUseNextAbility();
		return;
	}

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		TryUseNextAbility();
		return;
	}
	
	CurrentActivationASC = Data->AbilityOwnerASC;
	AActor* Target = TileManagerSubsystem->GetActorOnTile(Data->TargetTile.Get());
	Data->Payload.Instigator = CurrentActivationASC->GetAvatarActor();
	Data->Payload.Target = Target;
	
	CurrentActivationASC->OnAbilityEnded.AddUObject(this, &ThisClass::OnAbilityEnded);
	const bool bSuccess = CurrentActivationASC->TriggerAbilityFromGameplayEvent(Data->AbilitySpecHandle, CurrentActivationASC->AbilityActorInfo.Get(), Data->AbilityTag, &Data->Payload, *CurrentActivationASC);
	if (!bSuccess)
	{
		TryUseNextAbility();
	}
}

void UAbilityExecutionManagerComponent::OnAbilityEnded(const FAbilityEndedData& AbilityEndedData)
{
	if (CurrentActivationASC.IsValid())
	{
		CurrentActivationASC->OnAbilityEnded.RemoveAll(this);
		CurrentActivationASC.Reset();
	}

	TryUseNextAbility();
}
