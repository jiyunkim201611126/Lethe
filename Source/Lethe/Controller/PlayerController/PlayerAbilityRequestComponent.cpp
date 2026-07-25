// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAbilityRequestComponent.h"

#include "ActorSelectorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/AbilitySystem/Ability/LetheGameplayAbility.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"
#include "Lethe/SaveGame/SavedCardTypes.h"

UPlayerAbilityRequestComponent::UPlayerAbilityRequestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = true;
}

void UPlayerAbilityRequestComponent::InitializeComponent()
{
	Super::InitializeComponent();

	ActorSelector = GetOwner() ? GetOwner()->FindComponentByClass<UActorSelectorComponent>() : nullptr;
	check(ActorSelector.IsValid());
}

void UPlayerAbilityRequestComponent::ReserveMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const ATile* TargetTile)
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
	TileManagerSubsystem->FindShortestPath(TileManagerSubsystem->GetTileUnderActor(SelectedCharacter), TargetTile, OutPathTilesArray, false);

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
		CurrentReservedMove->PathTiles.Add(MakeWeakObjectPtr(Tile));
	}

	// 모든 ReservedMove를 검사해서 MoveRange가 있는 경우 대기 상태로 변경합니다.
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		if (const ICombatInterface* Combat = Cast<ICombatInterface>(ReservedMove.PlayerCharacter))
		{
			if (Combat->GetMoveRange() > 0)
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

void UPlayerAbilityRequestComponent::RemoveReservedMove(const AActor* SelectedCharacter)
{
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		if (ReservedMove.PlayerCharacter == SelectedCharacter)
		{
			for (const auto& Tile : ReservedMove.PathTiles)
			{
				if (Tile.IsValid())
				{
					Tile->SubtractOccupiedCount();
				}
			}
			ReservedMove.PathTiles.Reset();
			ReservedMove.State = EReservedMoveState::Finished;
			break;
		}
	}
}

bool UPlayerAbilityRequestComponent::TryEnqueueNextReservedMoveActivationContext()
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

		FAbilityActivationContext AbilityActivationContext;
		AbilityActivationContext.AbilitySpecHandle = AbilitySpecs[0]->Handle;
		if (ReachType == EMoveActionType::MoveAbility)
		{
			AbilityActivationContext.AbilityTag = LetheGameplayTags.Ability_Move;
		}
		else if (ReachType == EMoveActionType::SwapAbility)
		{
			AbilityActivationContext.AbilityTag = LetheGameplayTags.Ability_Swap;
			AbilityActivationContext.Payload.OptionalObject2 = OutSwapTargetReservedMove->PlayerCharacter.Get();
		}
		AbilityActivationContext.AbilityOwnerASC = ReservedMove.AbilitySystemComponent;

		AbilityActivationContext.PathTiles = PathTiles;
		
		LetheGameState->EnqueuePlayerAbilityActivationContext(MoveTemp(AbilityActivationContext), false);
		
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

EMoveActionType UPlayerAbilityRequestComponent::ResolveActionType(const FPlayerCharacterReservedMove* SourceReservedMove, TArray<TWeakObjectPtr<ATile>>& OutPathTiles, FPlayerCharacterReservedMove*& OutSwapTargetReservedMove)
{
	OutPathTiles.Reset();
	OutSwapTargetReservedMove = nullptr;

	if (!SourceReservedMove || !SourceReservedMove->IsValid() || SourceReservedMove->PathTiles.IsEmpty())
	{
		return EMoveActionType::CantReach;
	}

	if (const ICombatInterface* Combat = Cast<ICombatInterface>(SourceReservedMove->PlayerCharacter))
	{
		for (int32 Index = Combat->GetMoveRange() - 1; Index >= 0; --Index)
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

EMoveActionType UPlayerAbilityRequestComponent::GetActionType(const FPlayerCharacterReservedMove* SourceReservedMove, const ATile* TargetTile, FPlayerCharacterReservedMove*& OutSwapTargetReservedMove)
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

bool UPlayerAbilityRequestComponent::CanReachReservedTile(const FPlayerCharacterReservedMove* SourceReservedMove, const ATile* TargetTile) const
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

	const int32 MaxIndex = FMath::Min(Combat->GetMoveRange(), SourceReservedMove->PathTiles.Num()) - 1;
	for (int32 Index = 0; Index <= MaxIndex; ++Index)
	{
		if (SourceReservedMove->PathTiles[Index].Get() == TargetTile)
		{
			return true;
		}
	}

	return false;
}

void UPlayerAbilityRequestComponent::StartResolveMoves()
{
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState || LetheGameState->IsResolvingPlayerAbility())
	{
		return;
	}

	if (TryEnqueueNextReservedMoveActivationContext())
	{
		LetheGameState->StartActivatePlayerMoveAbilities();
	}
	else
	{
		LetheGameState->TryGoEnemyPlanningPhase();
	}
}

void UPlayerAbilityRequestComponent::OnPlayerReservedMoveResolved(AActor* MovedCharacter)
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

	TryEnqueueNextReservedMoveActivationContext();
}

void UPlayerAbilityRequestComponent::RefreshReservedMoveData(FPlayerCharacterReservedMove* ReservedMove) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}
	
	if (ReservedMove->IsValid())
	{
		// 이동 후, 캐싱된 경로에서 도달한 타일까지 제거 및 점유 카운트를 제거합니다.
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

		// MoveRange가 남아있다면 Moving 상태로 남겨서 StartResolveMoves에서 재평가하도록 합니다.
		if (const ICombatInterface* Combat = Cast<ICombatInterface>(ReservedMove->PlayerCharacter))
		{
			ReservedMove->State = Combat->GetMoveRange() <= 0 ? EReservedMoveState::Finished : EReservedMoveState::Moving;
		}

		// MoveRange가 남아있더라도, 최종 목적지에 도달했다면 Finished로 변경합니다.
		if (ReservedMove->PathTiles.IsEmpty())
		{
			ReservedMove->State = EReservedMoveState::Finished;
		}
	}
}

void UPlayerAbilityRequestComponent::SetAllReservedMovesWaitingForQueue()
{
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		ReservedMove.State = EReservedMoveState::WaitingForQueue;
	}
}

void UPlayerAbilityRequestComponent::ResetReservedMoveData()
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
	SwapSourceToTarget.Reset();
}

bool UPlayerAbilityRequestComponent::TryGetMovePathLocations(TMap<APlayerCharacterBase*, TArray<FVector>>& OutMovePathLocations) const
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

bool UPlayerAbilityRequestComponent::TryGetMovableTiles(AActor* SelectedCharacter, const UAbilitySystemComponent* AbilitySystemComponent, TArray<ATile*>& OutTilesInRange) const
{
	if (!SelectedCharacter || !AbilitySystemComponent || !ActorSelector.IsValid())
	{
		return false;
	}

	OutTilesInRange.Reset();
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
			MoveRange.Distance = Combat->GetMoveRange();
			ActorSelector->TryGetTilesByRangeFromActor(SelectedCharacter, MoveRange, ETileRangeQueryType::PlayerMove, OutTilesInRange);
		}
	}
	return !OutTilesInRange.IsEmpty();
}

void UPlayerAbilityRequestComponent::RequestMove(const AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const
{
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!SelectedCharacter || !AbilitySystemComponent || !TargetTile || !LetheGameState)
	{
		return;
	}

	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		const ATile* StartTile = TileManagerSubsystem->GetTileUnderActor(SelectedCharacter);
		if (!StartTile)
		{
			return;
		}
		
		if (TilesInRange.Contains(TargetTile))
		{
			if (TileManagerSubsystem->CanPlayerMoveToTile(TargetTile))
			{
				// 선택한 타일로 이동 가능한 경우 들어오는 분기입니다.
				const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
				TArray<FGameplayAbilitySpec*> AbilitySpecs;
				const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
				AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
				if (AbilitySpecs.IsEmpty())
				{
					return;
				}

				const ICombatInterface* SelectedCombatInterface = Cast<ICombatInterface>(SelectedCharacter);
				if (!SelectedCombatInterface)
				{
					return;
				}
			
				TArray<ATile*> OutPathTiles;
				if (TileManagerSubsystem->FindPrioritizedPathTiles(StartTile, TargetTile, SelectedCombatInterface->GetMoveRange(), OutPathTiles, false))
				{
					FAbilityActivationContext AbilityActivationContext;
					AbilityActivationContext.AbilitySpecHandle = AbilitySpecs[0]->Handle;
					AbilityActivationContext.AbilityTag = LetheGameplayTags.Ability_Move;
					AbilityActivationContext.AbilityOwnerASC = AbilitySystemComponent;
					
					for (ATile* PathTile : OutPathTiles)
					{
						if (PathTile)
						{
							AbilityActivationContext.PathTiles.Add(PathTile);
						}
					}
					LetheGameState->EnqueuePlayerAbilityActivationContext(MoveTemp(AbilityActivationContext));
				}
			}
			else
			{
				// 선택한 타일로 이동할 수 없는 경우, 스왑은 가능한지 확인합니다.
				const AActor* SwapTargetActor = TileManagerSubsystem->GetActorOnTile(TargetTile);
				const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(SwapTargetActor);
				if (!TargetCombatInterface)
				{
					return;
				}

				// 선택된 캐릭터는 이미 TargetTile로 이동할 MoveRange가 충분한 상태라는 게 검증됐기 때문에, TargetTile에 있는 캐릭터의 조건만 확인합니다.
				if (TargetCombatInterface->GetTeamSide() != ETeamSide::Player)
				{
					return;
				}
				
				const int32 TileDistance = TileManagerSubsystem->GetTileDistance(StartTile, TargetTile, EBFSType::Connection);
				if (TargetCombatInterface->GetMoveRange() < TileDistance)
				{
					return;
				}
				
				const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
				TArray<FGameplayAbilitySpec*> AbilitySpecs;
				const FGameplayTagContainer SwapTagContainer = LetheGameplayTags.Ability_Swap.GetSingleTagContainer();
				AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(SwapTagContainer, AbilitySpecs);
				if (AbilitySpecs.IsEmpty())
				{
					return;
				}
				
				TArray<ATile*> OutPathTiles;
				if (TileManagerSubsystem->FindPrioritizedPathTiles(StartTile, TargetTile, TargetCombatInterface->GetMoveRange(), OutPathTiles, true))
				{
					FAbilityActivationContext AbilityActivationContext;
					AbilityActivationContext.AbilitySpecHandle = AbilitySpecs[0]->Handle;
					AbilityActivationContext.AbilityTag = LetheGameplayTags.Ability_Swap;
					AbilityActivationContext.Payload.OptionalObject2 = SwapTargetActor;
					AbilityActivationContext.AbilityOwnerASC = AbilitySystemComponent;
					
					for (ATile* PathTile : OutPathTiles)
					{
						if (PathTile)
						{
							AbilityActivationContext.PathTiles.Add(PathTile);
						}
					}
					LetheGameState->EnqueuePlayerAbilityActivationContext(MoveTemp(AbilityActivationContext));
				}
			}
		}
	}
}

bool UPlayerAbilityRequestComponent::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const FGameplayTag& CardTag, int32 InHandIndex) const
{
	if (!ActorSelector.IsValid() || !OwnerASC || !AbilitySpecHandle.IsValid())
	{
		return false;
	}

	// 카드 사용 시엔 반드시 타일이 검출되어야 합니다.
	FTileHitResult OutTileHitResult;
	ActorSelector->GetTileHitResult(OutTileHitResult);
	if (!OutTileHitResult.Tile)
	{
		return false;
	}
	
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState)
	{
		return false;
	}

	const FGameplayAbilitySpec* AbilitySpec = OwnerASC->FindAbilitySpecFromHandle(AbilitySpecHandle);
	const ULetheCardAbility* CardAbility = AbilitySpec ? Cast<ULetheCardAbility>(AbilitySpec->Ability) : nullptr;
	const AActor* CardOwner = OwnerASC->GetAvatarActor();
	if (!CardAbility || !CardOwner)
	{
		return false;
	}

	FTargetingIntent TargetingIntent;
	TargetingIntent.HitTile = OutTileHitResult.Tile;
	TargetingIntent.ImpactPoint = OutTileHitResult.ImpactPoint;

	FAbilityActivationContext AbilityActivationContext;
	AbilityActivationContext.Index = InHandIndex;
	AbilityActivationContext.AbilitySpecHandle = AbilitySpecHandle;
	AbilityActivationContext.bCanUseOnTile = CardAbility->CanUseOnTile();
	AbilityActivationContext.AbilityTag = CardTag;
	AbilityActivationContext.AbilityOwnerASC = OwnerASC;

	FEffectTargetTileSelectorContext Context;
	Context.AvatarActor = CardOwner;
	Context.TargetingIntent = TargetingIntent;
	CardAbility->GetTargetTiles(Context);

	// 대상이 없고, 대상 없이 발동 불가능한 Ability인 경우 실패 처리합니다.
	if (!CardAbility->CanUseOnTile() && Context.OutTargetResults.IsEmpty())
	{
		return false;
	}

	AbilityActivationContext.TargetSelectionResults = MoveTemp(Context.OutTargetResults);
	LetheGameState->EnqueuePlayerAbilityActivationContext(MoveTemp(AbilityActivationContext));
	return true;
}

void UPlayerAbilityRequestComponent::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const
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
		const UCardDefinitionData* CardDefinitionData = Cast<UCardDefinitionData>(Spec->SourceObject);
		if (!CardAbility || !CardDefinitionData)
		{
			continue;
		}
		
		if (CardDefinitionData->CardId == SavedCard.CardId)
		{
			OutText = CardAbility->GetCardDescription(OwnerASC, SavedCard.CardLevel, CardDefinitionData->GetWeight(SavedCard.CardLevel));
			return;
		}
	}
}
