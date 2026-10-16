// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheMoveAbilityBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Algo/Reverse.h"
#include "Lethe/LetheLog.h"
#include "Lethe/AbilitySystem/AbilityTask/AbilityTask_WaitMoveArrived.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ULetheMoveAbilityBase::ULetheMoveAbilityBase()
{
	// 경로·타일을 실행마다 따로 들고 있어야 합니다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void ULetheMoveAbilityBase::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	for (const TPair<TWeakObjectPtr<ALetheCharacterBase>, TWeakObjectPtr<UAnimMontage>>& Pair : ExternalMoveMontages)
	{
		if (Pair.Key.IsValid() && Pair.Value.IsValid())
		{
			Pair.Key->StopAnimMontage(Pair.Value.Get());
		}
	}
	ExternalMoveMontages.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool ULetheMoveAbilityBase::InitMovePath(const FGameplayEventData* TriggerEventData)
{
	MovePathTiles.Reset();
	MoveStartTile = nullptr;
	MoveTargetTile = nullptr;

	const UMoveAbilityPayload* MovePayload = TriggerEventData ? Cast<UMoveAbilityPayload>(TriggerEventData->OptionalObject) : nullptr;
	if (!MovePayload || MovePayload->PathTiles.Num() < 2)
	{
		LETHE_LOG(LogAbility, Warning, "이동 경로 Payload가 없거나 시작·목적지 타일이 부족합니다.");
		return false;
	}

	MovePathTiles = MovePayload->PathTiles;
	MoveStartTile = MovePathTiles[0];
	MoveTargetTile = MovePathTiles.Last();
	return MoveStartTile && MoveTargetTile;
}

float ULetheMoveAbilityBase::GetPathSpeedMultiplier() const
{
	for (const ATile* PathTile : MovePathTiles)
	{
		if (PathTile && PathTile->GetTileVisionState() == ETileVisionState::Visible)
		{
			return 1.f;
		}
	}
	return HiddenPathSpeedMultiplier;
}

int32 ULetheMoveAbilityBase::GetMoveDistance() const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	return TileManagerSubsystem ? TileManagerSubsystem->GetTileDistance(MoveStartTile, MoveTargetTile, EBFSType::Connection) : 0;
}

void ULetheMoveAbilityBase::StartWalking(ALetheCharacterBase* MovingCharacter, const bool bReversePath, const float SpeedMultiplier) const
{
	if (!MovingCharacter)
	{
		return;
	}

	TArray<ATile*> PathTilesToMove;
	PathTilesToMove.Reserve(MovePathTiles.Num());
	for (ATile* PathTile : MovePathTiles)
	{
		PathTilesToMove.Add(PathTile);
	}
	if (bReversePath)
	{
		Algo::Reverse(PathTilesToMove);
	}

	MovingCharacter->SetMoveSpeedMultiplier(SpeedMultiplier);
	MovingCharacter->MoveToTile(PathTilesToMove, false);
}

void ULetheMoveAbilityBase::PlayMoveMontage(ALetheCharacterBase* MovingCharacter, const float PlayRate)
{
	UAnimMontage* MoveMontage = SelectMoveMontage(MovingCharacter);
	if (!MovingCharacter || !MoveMontage)
	{
		return;
	}

	if (MovingCharacter == GetAvatarActorFromActorInfo())
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MoveMontage, PlayRate);
		PlayMontageTask->ReadyForActivation();
		return;
	}

	if (MovingCharacter->PlayAnimMontage(MoveMontage, PlayRate) > 0.f)
	{
		ExternalMoveMontages.Emplace(MovingCharacter, MoveMontage);
	}
}

void ULetheMoveAbilityBase::WaitForArrival(const TArray<ALetheCharacterBase*>& MovingCharacters)
{
	UAbilityTask_WaitMoveArrived* WaitMoveArrivedTask = UAbilityTask_WaitMoveArrived::WaitMoveArrived(this, MovingCharacters, ArriveTimeoutSeconds);
	WaitMoveArrivedTask->OnArrived.AddDynamic(this, &ThisClass::OnMoveArrived);
	WaitMoveArrivedTask->ReadyForActivation();
}

void ULetheMoveAbilityBase::ApplyMoveCost(UAbilitySystemComponent* TargetASC) const
{
	if (!MoveCostEffectClass || !TargetASC)
	{
		return;
	}

	// Instigator는 항상 이 Ability의 주인입니다. 스왑 상대에게 적용하는 코스트도 마찬가지입니다.
	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(MoveCostEffectClass, 1.f);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 경로 길이가 아닌 두 타일 사이의 최단 거리로 소모량을 계산합니다(기존 GA_Move/GA_Swap 동작 유지).
	SpecHandle.Data->SetSetByCallerMagnitude(FLetheGameplayTags::Get().Attribute_Vital_MoveRange, static_cast<float>(-GetMoveDistance()));
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void ULetheMoveAbilityBase::FinishMove(const bool bSuccess)
{
	NotifyMoveResolved(bSuccess);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UAnimMontage* ULetheMoveAbilityBase::SelectMoveMontage(ALetheCharacterBase* MovingCharacter) const
{
	if (MoveMontageOverride)
	{
		return MoveMontageOverride;
	}
	return MovingCharacter ? MovingCharacter->GetMoveAnimation() : nullptr;
}

void ULetheMoveAbilityBase::OnMoveArrived(bool bTimedOut)
{
	FinishMove(true);
}
