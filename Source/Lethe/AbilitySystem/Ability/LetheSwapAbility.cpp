// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheSwapAbility.h"

#include "Lethe/LetheLog.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void ULetheSwapAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!InitMovePath(TriggerEventData) || !InitSwapTarget(TriggerEventData) || !CanSwap())
	{
		FinishMove(false);
		return;
	}

	ALetheCharacterBase* SwapSourceCharacter = Cast<ALetheCharacterBase>(GetAvatarActorFromActorInfo());
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!ensure(SwapSourceCharacter && TileManagerSubsystem))
	{
		FinishMove(false);
		return;
	}

	// 두 캐릭터의 논리 위치를 걷기 전에 맞바꿉니다. 목적지를 먼저 매핑하면 상대의 기존 매핑이 풀리므로 이 순서를 유지해야 합니다.
	TileManagerSubsystem->MapTileAndActor(MoveTargetTile, SwapSourceCharacter);
	TileManagerSubsystem->MapTileAndActor(MoveStartTile, SwapTargetCharacter);

	const float SpeedMultiplier = GetPathSpeedMultiplier();
	StartWalking(SwapSourceCharacter, false, SpeedMultiplier);
	StartWalking(SwapTargetCharacter, true, SpeedMultiplier);
	PlayMoveMontage(SwapSourceCharacter, SpeedMultiplier);
	PlayMoveMontage(SwapTargetCharacter, SpeedMultiplier);
	WaitForArrival({ SwapSourceCharacter, SwapTargetCharacter });
}

void ULetheSwapAbility::NotifyMoveResolved(const bool bSuccess)
{
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState)
	{
		LETHE_LOG(LogAbility, Error, "LetheGameState를 찾지 못해 스왑 결과를 알리지 못했습니다.");
		return;
	}

	// 코스트와 소음은 실제로 자리를 바꾼 경우에만 적용합니다. 실패해도 예약 이동 흐름이 이어지도록 결과는 항상 알립니다.
	if (bSuccess)
	{
		ApplyMoveCost(GetAbilitySystemComponentFromActorInfo());
		ApplyMoveCost(SwapTargetCharacter ? SwapTargetCharacter->GetAbilitySystemComponent() : nullptr);
		ActivateNoise(MoveStartTile, { MoveTargetTile.Get() });
		ActivateNoise(MoveTargetTile, { MoveStartTile.Get() });
	}
	LetheGameState->NotifyPlayerMoveResolved(GetAvatarActorFromActorInfo());
}

bool ULetheSwapAbility::InitSwapTarget(const FGameplayEventData* TriggerEventData)
{
	// GAS Payload의 OptionalObject는 const로만 전달되지만, 스왑 상대는 실제로 움직여야 하므로 const를 벗깁니다.
	SwapTargetCharacter = TriggerEventData ? const_cast<ALetheCharacterBase*>(Cast<ALetheCharacterBase>(TriggerEventData->OptionalObject2)) : nullptr;
	if (!SwapTargetCharacter || SwapTargetCharacter == GetAvatarActorFromActorInfo())
	{
		LETHE_LOG(LogAbility, Warning, "스왑 상대가 없거나 자기 자신입니다.");
		SwapTargetCharacter = nullptr;
		return false;
	}
	return true;
}

bool ULetheSwapAbility::CanSwap() const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	const ICombatInterface* SourceCombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo());
	if (!TileManagerSubsystem || !SourceCombatInterface)
	{
		return false;
	}

	if (TileManagerSubsystem->GetActorOnTile(MoveTargetTile) != SwapTargetCharacter)
	{
		LETHE_LOG(LogAbility, Warning, "스왑 상대가 목적지 타일에 없습니다.");
		return false;
	}

	// 두 캐릭터 모두 같은 거리를 걷고 같은 코스트를 내므로, 양쪽 이동력을 모두 확인합니다.
	const int32 MoveDistance = GetMoveDistance();
	if (SourceCombatInterface->GetMoveRange() < MoveDistance || SwapTargetCharacter->GetMoveRange() < MoveDistance)
	{
		LETHE_LOG(LogAbility, Warning, "MoveRange보다 더 먼 거리의 캐릭터와 스왑을 시도했습니다.");
		return false;
	}
	return true;
}
