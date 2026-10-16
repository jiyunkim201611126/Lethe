// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheMoveAbility.h"

#include "Lethe/LetheLog.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void ULetheMoveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!InitMovePath(TriggerEventData) || !CanMoveToTargetTile())
	{
		FinishMove(false);
		return;
	}

	ALetheCharacterBase* MovingCharacter = Cast<ALetheCharacterBase>(GetAvatarActorFromActorInfo());
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!ensure(MovingCharacter && TileManagerSubsystem))
	{
		FinishMove(false);
		return;
	}

	// 논리 위치는 걷기 전에 확정합니다. 시각적 위치(시야 판정용)는 캐릭터가 걸으며 따로 갱신합니다.
	TileManagerSubsystem->MapTileAndActor(MoveTargetTile, MovingCharacter);

	const float SpeedMultiplier = GetPathSpeedMultiplier();
	StartWalking(MovingCharacter, false, SpeedMultiplier);
	PlayMoveMontage(MovingCharacter, SpeedMultiplier);
	WaitForArrival({ MovingCharacter });
}

void ULetheMoveAbility::NotifyMoveResolved(const bool bSuccess)
{
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState)
	{
		LETHE_LOG(LogAbility, Error, "LetheGameState를 찾지 못해 이동 결과를 알리지 못했습니다.");
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	if (AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		// 코스트와 소음은 실제로 이동한 경우에만 적용합니다. 실패해도 예약 이동 흐름이 이어지도록 결과는 항상 알립니다.
		if (bSuccess)
		{
			ApplyMoveCost(GetAbilitySystemComponentFromActorInfo());
			ActivateNoise(MoveStartTile, { MoveTargetTile.Get() });
		}
		LetheGameState->NotifyPlayerMoveResolved(AvatarActor);
	}
	else if (const AEnemyCharacterBase* EnemyCharacter = Cast<AEnemyCharacterBase>(AvatarActor))
	{
		// 적은 성공 여부와 관계없이 다음 계획 단계로 넘어갑니다(기존 GA_Move 동작 유지).
		EnemyCharacter->ProcessCommitPlan();
	}
}

bool ULetheMoveAbility::CanMoveToTargetTile() const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	const ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo());
	if (!TileManagerSubsystem || !CombatInterface)
	{
		return false;
	}

	if (TileManagerSubsystem->GetActorOnTile(MoveTargetTile))
	{
		LETHE_LOG(LogAbility, Warning, "액터가 존재하는 타일 위로 이동을 시도했습니다.");
		return false;
	}

	// 경로에 시작 타일이 포함되어 있으므로 실제 이동 칸 수는 1을 뺀 값입니다.
	if (CombatInterface->GetMoveRange() < MovePathTiles.Num() - 1)
	{
		LETHE_LOG(LogAbility, Warning, "MoveRange보다 더 먼 거리의 타일로 이동을 시도했습니다.");
		return false;
	}
	return true;
}
