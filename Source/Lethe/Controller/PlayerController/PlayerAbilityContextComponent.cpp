// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAbilityContextComponent.h"

#include "ActorSelectorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/AbilitySystem/Ability/LetheGameplayAbility.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

UPlayerAbilityContextComponent::UPlayerAbilityContextComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UPlayerAbilityContextComponent::InitializeComponent()
{
	Super::InitializeComponent();

	ActorSelector = GetOwner() ? GetOwner()->FindComponentByClass<UActorSelectorComponent>() : nullptr;
	check(ActorSelector.IsValid());
}

bool UPlayerAbilityContextComponent::TryGetMovableTiles(AActor* SelectedCharacter, const UAbilitySystemComponent* AbilitySystemComponent, TArray<ATile*>& OutTilesInRange) const
{
	OutTilesInRange.Reset();
	if (!SelectedCharacter || !AbilitySystemComponent || !ActorSelector.IsValid())
	{
		return false;
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	TArray<FGameplayAbilitySpec*> AbilitySpecs;
	const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
	AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
	if (!AbilitySpecs.IsEmpty())
	{
		if (const ICombatInterface* Combat = Cast<ICombatInterface>(SelectedCharacter))
		{
			FBFSRange MoveRange;
			MoveRange.BFSType = EBFSType::Connection;
			MoveRange.Distance = Combat->GetMoveDistance();
			ActorSelector->TryGetTilesByDepth(OutTilesInRange, SelectedCharacter, MoveRange);
		}
	}
	return !OutTilesInRange.IsEmpty();
}

void UPlayerAbilityContextComponent::ReserveMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const ATile* TargetTile)
{
	if (!SelectedCharacter || !AbilitySystemComponent || !TargetTile)
	{
		return;
	}

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	// 이미 예약된 이동이 있다면 가져오고, 없다면 생성합니다.
	FPlayerCharacterReservedMove* CurrentReservedMove = ReservedMoves.FindByPredicate([SelectedCharacter](const FPlayerCharacterReservedMove& InReservedMove)
	{
		return InReservedMove.PlayerCharacter == SelectedCharacter;
	});
	if (!CurrentReservedMove)
	{
		CurrentReservedMove = &ReservedMoves.Emplace_GetRef();
		CurrentReservedMove->PlayerCharacter = SelectedCharacter;
		CurrentReservedMove->AbilitySystemComponent = AbilitySystemComponent;
	}

	// 예약 전, 값을 초기화합니다.
	for (const auto& Tile : CurrentReservedMove->PathTiles)
	{
		if (Tile.IsValid())
		{
			Tile->SubtractOccupiedCount();
		}
	}
	CurrentReservedMove->PathTiles.Reset();
	CurrentReservedMove->State = EReservedMoveState::Finished;

	// TargetTile을 향한 모든 가능 경로를 가져옵니다.
	TArray<TArray<ATile*>> OutPathTilesArray;
	TileManagerSubsystem->FindShortestPath(TileManagerSubsystem->GetTileUnderActor(SelectedCharacter), TargetTile, OutPathTilesArray, true);

	int32 MinOccupiedCount = INT32_MAX;
	int32 SelectedPathIndex = INDEX_NONE;
	for (int32 PathIndex = 0; PathIndex < OutPathTilesArray.Num(); ++PathIndex)
	{
		// 만들어진 모든 경로를 탐색해 가장 우선도가 높은(다른 캐릭터가 덜 지나치는) 경로를 선택합니다.
		int32 CurrentOccupiedCount = 0;
		for (const ATile* Tile : OutPathTilesArray[PathIndex])
		{
			CurrentOccupiedCount += Tile->GetOccupiedCount();
			if (TileManagerSubsystem->GetActorOnTile(Tile))
			{
				// 타일 위에 캐릭터가 서있다면 가중치를 1 추가합니다.
				++CurrentOccupiedCount;
			}
		}

		// 현재 경로를 아무도 지나지 않는 상태라면 이 경로를 선택합니다.
		if (CurrentOccupiedCount <= 0)
		{
			SelectedPathIndex = PathIndex;
			break;
		}

		// 가장 OccupiedCount가 낮은 경로를 선택합니다.
		if (CurrentOccupiedCount < MinOccupiedCount)
		{
			MinOccupiedCount = CurrentOccupiedCount;
			SelectedPathIndex = PathIndex;
		}
	}

	if (SelectedPathIndex == INDEX_NONE)
	{
		return;
	}

	// 선택된 경로를 캐싱합니다.
	for (ATile* Tile : OutPathTilesArray[SelectedPathIndex])
	{
		Tile->AddOccupiedCount();
		CurrentReservedMove->PathTiles.Emplace(MakeWeakObjectPtr(Tile));
	}

	// 모든 ReservedMove를 검사해서 MoveDistance가 있는 경우 대기 상태로 변경합니다.
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		if (const ICombatInterface* Combat = Cast<ICombatInterface>(ReservedMove.PlayerCharacter))
		{
			if (Combat->GetMoveDistance() > 0)
			{
				ReservedMove.State = EReservedMoveState::WaitingForQueue;
			}
		}
	}

	// 이동 예약을 수정했으므로, 턴 종료 버튼 클릭 시 잔여 행동력 여부를 한 번 점검해야 합니다.
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->SetShouldDeferEndPlayerMovePhase();
	}
}

void UPlayerAbilityContextComponent::RemoveReservedMove(const AActor* SelectedCharacter)
{
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		if (ReservedMove.PlayerCharacter == SelectedCharacter)
		{
			ReservedMove.PathTiles.Reset();
			break;
		}
	}
}

bool UPlayerAbilityContextComponent::TryEnqueueNextReservedMoveActivationData()
{
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState)
	{
		return false;
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
	const FGameplayTagContainer SwapTagContainer = LetheGameplayTags.Ability_Swap.GetSingleTagContainer();
	
	bool bIsActivationDataAdded = false;
	bool bAnyMoveWaitingForUnblock = false;
	TArray<int32> DelayIndices;
	for (int32 Index = 0; Index < ReservedMoves.Num(); ++Index)
	{
		FPlayerCharacterReservedMove& ReservedMove = ReservedMoves[Index];
		if (!ReservedMove.IsValid() || ReservedMove.State == EReservedMoveState::Finished)
		{
			continue;
		}

		if (ReservedMove.bIsSwapTarget)
		{
			ReservedMove.bIsSwapTarget = false;
			continue;
		}

		TArray<TWeakObjectPtr<ATile>> PathTiles;
		FPlayerCharacterReservedMove* OutSwapTargetReservedMove = nullptr;
		EMoveActionType ReachType = ResolveActionType(&ReservedMove, PathTiles, OutSwapTargetReservedMove);

		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		switch (ReachType)
		{
		case EMoveActionType::CantReach:
			{
				// 도달할 수 없는 상태라면 수행 순서를 뒤로 미루기 위해 기록합니다.
				DelayIndices.Emplace(Index);
				ReservedMove.State = EReservedMoveState::WaitingForUnblock;
				bAnyMoveWaitingForUnblock = true;
			}
			continue;
		case EMoveActionType::MoveAbility:
			{
				// GA_Move를 통한 이동을 시작합니다.
				ReservedMove.AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
				if (AbilitySpecs.IsEmpty())
				{
					ReservedMove.State = EReservedMoveState::Finished;
					continue;
				}
			}
			break;
		case EMoveActionType::SwapAbility:
			{
				// GA_Swap을 통한 이동을 시작합니다.
				ReservedMove.AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(SwapTagContainer, AbilitySpecs);
				if (AbilitySpecs.IsEmpty() || !OutSwapTargetReservedMove)
				{
					ReservedMove.State = EReservedMoveState::Finished;
					continue;
				}
				OutSwapTargetReservedMove->bIsSwapTarget = true;
				SwapSourceToTarget.Emplace(ReservedMove.PlayerCharacter, OutSwapTargetReservedMove->PlayerCharacter);
			}
			break;
		}

		// 여기까지 내려온 경우, 이 캐릭터의 이동을 시작합니다.
		ReservedMove.State = EReservedMoveState::Moving;

		FAbilityActivationData AbilityActivationData;
		AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
		if (ReachType == EMoveActionType::MoveAbility)
		{
			AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
		}
		else if (ReachType == EMoveActionType::SwapAbility)
		{
			AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Swap;
			AbilityActivationData.Payload.OptionalObject2 = OutSwapTargetReservedMove->PlayerCharacter.Get();
		}
		AbilityActivationData.AbilityOwnerASC = ReservedMove.AbilitySystemComponent;
		AbilityActivationData.TargetTiles = MoveTemp(PathTiles);
		LetheGameState->EnqueuePlayerAbilityActivationData(AbilityActivationData, false);
		
		bIsActivationDataAdded = true;
		break;
	}

	// 무한 루프 상태를 방지하기 위해, 이번에 움직일 수 있는 캐릭터가 아무도 없다면 모든 이동 예약을 마무리합니다.
	if (!bIsActivationDataAdded && bAnyMoveWaitingForUnblock)
	{
		for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
		{
			if (ReservedMove.State == EReservedMoveState::WaitingForUnblock)
			{
				ReservedMove.State = EReservedMoveState::Finished;
			}
		}
		return bIsActivationDataAdded;
	}

	// DelayIndex가 오름차순으로 정렬되어 있으므로, 역순으로 순회하며 ReservedMoves에서 제거한 후 뒤쪽에 붙입니다.
	TArray<FPlayerCharacterReservedMove> DelayReservedMoves;
	DelayReservedMoves.Reserve(DelayIndices.Num());
	for (int32 Index = DelayIndices.Num() - 1; Index >= 0; --Index)
	{
		const int32 ReservedMoveIndex = DelayIndices[Index];
		DelayReservedMoves.Insert(ReservedMoves[ReservedMoveIndex], 0);
		ReservedMoves.RemoveAt(ReservedMoveIndex);
	}
	ReservedMoves.Append(DelayReservedMoves);

	return bIsActivationDataAdded;
}

EMoveActionType UPlayerAbilityContextComponent::ResolveActionType(const FPlayerCharacterReservedMove* SourceReservedMove, TArray<TWeakObjectPtr<ATile>>& OutPathTiles, FPlayerCharacterReservedMove*& OutSwapTargetReservedMove)
{
	OutPathTiles.Reset();
	OutSwapTargetReservedMove = nullptr;

	if (!SourceReservedMove || !SourceReservedMove->IsValid() || SourceReservedMove->PathTiles.IsEmpty())
	{
		return EMoveActionType::CantReach;
	}

	if (const ICombatInterface* Combat = Cast<ICombatInterface>(SourceReservedMove->PlayerCharacter))
	{
		for (int32 Index = Combat->GetMoveDistance() - 1; Index >= 0; --Index)
		{
			const auto& CandidateTile = SourceReservedMove->PathTiles.IsValidIndex(Index) ? SourceReservedMove->PathTiles[Index] : nullptr;
			const EMoveActionType ReachType = GetActionType(SourceReservedMove, CandidateTile.Get(), OutSwapTargetReservedMove);
			if (ReachType != EMoveActionType::CantReach)
			{
				for (int32 PathTileIndex = 0; PathTileIndex <= Index; ++PathTileIndex)
				{
					if (SourceReservedMove->PathTiles.IsValidIndex(PathTileIndex))
					{
						OutPathTiles.Emplace(SourceReservedMove->PathTiles[PathTileIndex]);
					}
				}
				return ReachType;
			}
		}
	}
	return EMoveActionType::CantReach;
}

EMoveActionType UPlayerAbilityContextComponent::GetActionType(const FPlayerCharacterReservedMove* SourceReservedMove, const ATile* TargetTile, FPlayerCharacterReservedMove*& OutSwapTargetReservedMove)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!SourceReservedMove || !SourceReservedMove->IsValid() || !TargetTile || SourceReservedMove->PathTiles.IsEmpty() || !TileManagerSubsystem)
	{
		return EMoveActionType::CantReach;
	}

	if (TileManagerSubsystem->CanPlayerMoveToTile(TargetTile))
	{
		return EMoveActionType::MoveAbility;
	}

	// TargetTile 위에 액터가 있는 경우 들어가는 분기입니다.
	if (const AActor* ActorOnTargetTile = TileManagerSubsystem->GetActorOnTile(TargetTile))
	{
		const ATile* SourceCurrentTile = TileManagerSubsystem->GetTileUnderActor(SourceReservedMove->PlayerCharacter.Get());
		if (!SourceCurrentTile)
		{
			return EMoveActionType::CantReach;
		}

		// 예약된 모든 이동을 순회합니다.
		for (auto& TargetReservedMove : ReservedMoves)
		{
			// TargetTile 위에 존재하는 액터가 이동을 예약한 캐릭터인 경우 들어가는 분기입니다.
			if (ActorOnTargetTile == TargetReservedMove.PlayerCharacter)
			{
				if (CanReachReservedTile(&TargetReservedMove, SourceCurrentTile))
				{
					// Target 캐릭터가 Source의 타일로 도달 가능한 상태면 Swap을 반환합니다.
					OutSwapTargetReservedMove = &TargetReservedMove;
					return EMoveActionType::SwapAbility;
				}

				return EMoveActionType::CantReach;
			}
		}
	}
	return EMoveActionType::CantReach;
}

bool UPlayerAbilityContextComponent::CanReachReservedTile(const FPlayerCharacterReservedMove* SourceReservedMove, const ATile* TargetTile) const
{
	if (!SourceReservedMove || !SourceReservedMove->IsValid() || !TargetTile)
	{
		return false;
	}

	const ICombatInterface* Combat = Cast<ICombatInterface>(SourceReservedMove->PlayerCharacter.Get());
	if (!Combat)
	{
		return false;
	}

	const int32 MaxIndex = FMath::Min(Combat->GetMoveDistance(), SourceReservedMove->PathTiles.Num()) - 1;
	for (int32 Index = 0; Index <= MaxIndex; ++Index)
	{
		if (SourceReservedMove->PathTiles[Index].Get() == TargetTile)
		{
			return true;
		}
	}

	return false;
}

void UPlayerAbilityContextComponent::StartResolveMoves()
{
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState || LetheGameState->IsResolvingPlayerAbility())
	{
		return;
	}

	if (TryEnqueueNextReservedMoveActivationData())
	{
		LetheGameState->StartActivatePlayerMoveAbilities();
	}
	else
	{
		LetheGameState->TryGoEnemyPlanningPhase();
	}
}

void UPlayerAbilityContextComponent::OnPlayerMoveResolved(AActor* MovedCharacter)
{
	FPlayerCharacterReservedMove* SourceReservedMove = ReservedMoves.FindByPredicate([MovedCharacter](const FPlayerCharacterReservedMove& InReservedMove)
	{
		return InReservedMove.PlayerCharacter == MovedCharacter;
	});
	if (!SourceReservedMove || !SourceReservedMove->IsValid())
	{
		return;
	}

	if (SourceReservedMove->PlayerCharacter == MovedCharacter)
	{
		RefreshReservedMoveData(SourceReservedMove);
	}

	TWeakObjectPtr<AActor> SwappedTarget;
	if (SwapSourceToTarget.RemoveAndCopyValue(MovedCharacter, SwappedTarget))
	{
		if (SwappedTarget.IsValid())
		{
			for (FPlayerCharacterReservedMove& TargetReservedMove : ReservedMoves)
			{
				if (TargetReservedMove.PlayerCharacter == SwappedTarget)
				{
					TargetReservedMove.bIsSwapTarget = false;
					RefreshReservedMoveData(&TargetReservedMove);
				}
			}
		}
	}

	TryEnqueueNextReservedMoveActivationData();
}

void UPlayerAbilityContextComponent::RefreshReservedMoveData(FPlayerCharacterReservedMove* ReservedMove) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}
	
	if (ReservedMove->IsValid())
	{
		// 이동 후, 캐싱된 경로에서 도달한 타일까지 제거합니다.
		ATile* CurrentTile = TileManagerSubsystem->GetTileUnderActor(ReservedMove->PlayerCharacter.Get());
		if (!CurrentTile)
		{
			return;
		}

		const int32 ReachedIndex = ReservedMove->PathTiles.IndexOfByKey(CurrentTile);
		if (ReachedIndex != INDEX_NONE)
		{
			const int32 RemoveCount = ReachedIndex + 1;
			for (int32 Index = 0; Index < RemoveCount; ++Index)
			{
				if (ReservedMove->PathTiles.IsValidIndex(Index))
				{
					const auto& Tile = ReservedMove->PathTiles[Index];
					if (Tile.IsValid())
					{
						Tile->SubtractOccupiedCount();
					}
				}
			}
			ReservedMove->PathTiles.RemoveAt(0, RemoveCount);
		}

		// MoveDistance가 남아있다면 Moving 상태로 남겨서 StartResolveMoves에서 재평가하도록 합니다.
		if (const ICombatInterface* Combat = Cast<ICombatInterface>(ReservedMove->PlayerCharacter))
		{
			ReservedMove->State = Combat->GetMoveDistance() <= 0 ? EReservedMoveState::Finished : EReservedMoveState::Moving;
		}

		// MoveDistance가 남아있더라도, 최종 목적지에 도달했다면 Finished로 변경합니다.
		if (ReservedMove->PathTiles.IsEmpty())
		{
			ReservedMove->State = EReservedMoveState::Finished;
		}
	}
}

void UPlayerAbilityContextComponent::SetAllReservedMovesWaitingForQueue()
{
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		ReservedMove.State = EReservedMoveState::WaitingForQueue;
	}
}

void UPlayerAbilityContextComponent::ResetReservedMoveData()
{
	for (const FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		for (const auto& Tile : ReservedMove.PathTiles)
		{
			if (Tile.IsValid())
			{
				Tile->SubtractOccupiedCount();
			}
		}
	}
	ReservedMoves.Reset();
}

bool UPlayerAbilityContextComponent::TryGetMovePathLocations(TMap<APlayerCharacterBase*, TArray<FVector>>& OutMovePathLocations) const
{
	OutMovePathLocations.Reset();

	const FVector MovePreviewLocationOffset = FVector(0.f, 0.f, 1.f);
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return false;
	}
	
	for (const FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		if (!ReservedMove.IsValid())
		{
			continue;
		}

		if (APlayerCharacterBase* ReservedCharacter = Cast<APlayerCharacterBase>(ReservedMove.PlayerCharacter))
		{
			TArray<FVector>& TileLocations = OutMovePathLocations.FindOrAdd(ReservedCharacter);
			TileLocations.Reserve(ReservedMove.PathTiles.Num() + 1);
			if (const ATile* PlayerCharacterTile = TileManagerSubsystem->GetTileUnderActor(ReservedMove.PlayerCharacter.Get()))
			{
				TileLocations.Emplace(PlayerCharacterTile->GetActorLocation() + MovePreviewLocationOffset);
				for (const auto& PathTile : ReservedMove.PathTiles)
				{
					if (PathTile.IsValid())
					{
						TileLocations.Emplace(PathTile->GetActorLocation() + MovePreviewLocationOffset);
					}
				}
			}
		}
	}
	return !OutMovePathLocations.IsEmpty();
}

void UPlayerAbilityContextComponent::RequestMove(const AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const
{
	if (!SelectedCharacter || !AbilitySystemComponent || !TargetTile)
	{
		return;
	}

	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (TilesInRange.Contains(TargetTile) && TileManagerSubsystem->CanPlayerMoveToTile(TargetTile))
		{
			// 선택한 타일로 이동 가능한 경우 들어오는 분기입니다.
			if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
			{
				const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
				TArray<FGameplayAbilitySpec*> AbilitySpecs;
				const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
				AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
				if (AbilitySpecs.IsEmpty())
				{
					return;
				}

				const ATile* StartTile = TileManagerSubsystem->GetTileUnderActor(SelectedCharacter);
				if (!StartTile)
				{
					return;
				}

				const ICombatInterface* CombatInterface = Cast<ICombatInterface>(SelectedCharacter);
				if (!CombatInterface)
				{
					return;
				}
				
				TArray<ATile*> OutPathTiles;
				TileManagerSubsystem->FindPrioritizedPathTiles(StartTile, TargetTile, CombatInterface->GetMoveDistance(), OutPathTiles, false);

				FAbilityActivationData AbilityActivationData;
				AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
				AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
				AbilityActivationData.AbilityOwnerASC = AbilitySystemComponent;
				for (ATile* PathTile : OutPathTiles)
				{
					if (PathTile)
					{
						AbilityActivationData.TargetTiles.Emplace(PathTile);
					}
				}
				LetheGameState->EnqueuePlayerAbilityActivationData(AbilityActivationData);
			}
		}
	}
}

bool UPlayerAbilityContextComponent::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, int32 InHandIndex) const
{
	if (!ActorSelector.IsValid() || !OwnerASC)
	{
		return false;
	}

	// 카드 사용 시엔 검출된 타일 위에 반드시 액터가 있어야 합니다.
	FTileAndActor OutTileAndActor;
	ActorSelector->GetTileAndActorUnderCursor(OutTileAndActor);
	if (!OutTileAndActor.Tile || !OutTileAndActor.Actor)
	{
		return false;
	}

	// CardTag에 해당하는 AbilitySpec을 모두 가져옵니다.
	TArray<FGameplayAbilitySpec*> AbilitySpecs;
	const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
	OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);
	if (AbilitySpecs.IsEmpty())
	{
		return false;
	}
	
	// TODO: 중복 카드가 있다면 AbilitySpec이 여러 개 나오므로, 추후 CardLevel로 알맞은 Ability인지 확인하는 과정이 필요할 수 있습니다.
	// TODO: 현재는 첫 번째 거로 사용합니다.
	
	const ULetheCardAbility* CardAbility = Cast<ULetheCardAbility>(AbilitySpecs[0]->Ability);
	if (!CardAbility)
	{
		return false;
	}

	// Ability 사용 범위 내의 타일을 선택했는지 확인합니다.
	TArray<ATile*> OutTiles;
	ActorSelector->TryGetTilesByDepth(OutTiles, OwnerASC->GetAvatarActor(), CardAbility->GetAbilityRange());
	if (!OutTiles.Contains(OutTileAndActor.Tile))
	{
		return false;
	}

	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState)
	{
		return false;
	}

	FAbilityActivationData AbilityActivationData;
	AbilityActivationData.Index = InHandIndex;
	AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
	AbilityActivationData.AbilityTag = CardTag;
	AbilityActivationData.AbilityOwnerASC = OwnerASC;
	AbilityActivationData.TargetTiles.Emplace(OutTileAndActor.Tile);
	LetheGameState->EnqueuePlayerAbilityActivationData(AbilityActivationData);
	return true;
}

void UPlayerAbilityContextComponent::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const
{
	if (!OwnerASC)
	{
		return;
	}

	TArray<FGameplayAbilitySpecHandle> OutAbilityHandles;
	OwnerASC->GetAllAbilities(OutAbilityHandles);
	for (const FGameplayAbilitySpecHandle& Handle : OutAbilityHandles)
	{
		const FGameplayAbilitySpec* Spec = OwnerASC->FindAbilitySpecFromHandle(Handle);
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		const ULetheCardAbility* CardAbility = Cast<ULetheCardAbility>(Spec->Ability);
		if (CardAbility && CardAbility->GetAssetTags().HasAllExact(CardTag.GetSingleTagContainer()))
		{
			OutText = CardAbility->GetCardDescription(OwnerASC, 1);
			return;
		}
	}
}
