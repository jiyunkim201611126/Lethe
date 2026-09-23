// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheMoveAbility.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Lethe/LetheLog.h"
#include "Lethe/AbilitySystem/AbilityTask/AbilityTask_WaitMoveArrived.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ULetheMoveAbility::ULetheMoveAbility()
{
	// 경로·타일을 실행마다 따로 들고 있어야 합니다.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void ULetheMoveAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!InitMovePath(TriggerEventData) || !CanMoveToTargetTile())
	{
		FinishMove(false);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	ALetheCharacterBase* MovingCharacter = Cast<ALetheCharacterBase>(AvatarActor);
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!ensure(MovingCharacter && TileManagerSubsystem))
	{
		FinishMove(false);
		return;
	}

	// 논리 위치는 걷기 전에 확정합니다. 시각적 위치(시야 판정용)는 캐릭터가 걸으며 따로 갱신합니다.
	TileManagerSubsystem->MapTileAndActor(MoveTargetTile, AvatarActor);

	const float SpeedMultiplier = IsPathVisible() ? 1.f : HiddenPathSpeedMultiplier;
	MovingCharacter->SetMoveSpeedMultiplier(SpeedMultiplier);

	TArray<ATile*> PathTilesToMove;
	PathTilesToMove.Reserve(MovePathTiles.Num());
	for (ATile* PathTile : MovePathTiles)
	{
		PathTilesToMove.Add(PathTile);
	}
	MovingCharacter->MoveToTile(PathTilesToMove, false);

	if (UAnimMontage* MoveMontage = SelectMoveMontage())
	{
		UAbilityTask_PlayMontageAndWait* PlayMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, MoveMontage, SpeedMultiplier);
		PlayMontageTask->ReadyForActivation();
	}

	// 몽타주보다 뒤에 등록합니다. 이미 도착해 있으면 즉시 종료되며, 그 경우 몽타주도 함께 정리됩니다.
	UAbilityTask_WaitMoveArrived* WaitMoveArrivedTask = UAbilityTask_WaitMoveArrived::WaitMoveArrived(this, { MovingCharacter }, ArriveTimeoutSeconds);
	WaitMoveArrivedTask->OnArrived.AddDynamic(this, &ThisClass::OnMoveArrived);
	WaitMoveArrivedTask->ReadyForActivation();
}

bool ULetheMoveAbility::InitMovePath(const FGameplayEventData* TriggerEventData)
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

bool ULetheMoveAbility::IsPathVisible() const
{
	for (const ATile* PathTile : MovePathTiles)
	{
		if (PathTile && PathTile->GetTileVisionState() == ETileVisionState::Visible)
		{
			return true;
		}
	}
	return false;
}

UAnimMontage* ULetheMoveAbility::SelectMoveMontage() const
{
	if (MoveMontageOverride)
	{
		return MoveMontageOverride;
	}

	if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(GetAvatarActorFromActorInfo()))
	{
		return CombatInterface->GetMoveAnimation();
	}
	return nullptr;
}

void ULetheMoveAbility::ApplyMoveCost() const
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!MoveCostEffectClass || !OwnerASC || !TileManagerSubsystem)
	{
		return;
	}

	const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(MoveCostEffectClass, 1.f);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 경로 길이가 아닌 두 타일 사이의 최단 거리로 소모량을 계산합니다(기존 GA_Move 동작 유지).
	const int32 MoveDistance = TileManagerSubsystem->GetTileDistance(MoveStartTile, MoveTargetTile, EBFSType::Connection);
	SpecHandle.Data->SetSetByCallerMagnitude(FLetheGameplayTags::Get().Attribute_Vital_MoveRange, static_cast<float>(-MoveDistance));
	OwnerASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
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
			ApplyMoveCost();
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

void ULetheMoveAbility::OnMoveArrived(bool bTimedOut)
{
	FinishMove(true);
}

void ULetheMoveAbility::FinishMove(const bool bSuccess)
{
	NotifyMoveResolved(bSuccess);
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}
