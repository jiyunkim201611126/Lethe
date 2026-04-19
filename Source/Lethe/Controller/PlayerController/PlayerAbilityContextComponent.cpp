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
	FPlayerCharacterReservedMove* ReservedMove = ReservedMoves.FindByPredicate([SelectedCharacter](const FPlayerCharacterReservedMove& InReservedMove)
	{
		return InReservedMove.PlayerCharacter == SelectedCharacter;
	});
	if (!ReservedMove)
	{
		ReservedMove = &ReservedMoves.Emplace_GetRef();
		ReservedMove->PlayerCharacter = SelectedCharacter;
		ReservedMove->AbilitySystemComponent = AbilitySystemComponent;
	}

	// 예약 전, 값을 초기화합니다.
	for (const auto& Tile : ReservedMove->PathTiles)
	{
		if (Tile.IsValid())
		{
			Tile->SubtractOccupiedCount();
		}
	}
	ReservedMove->PathTiles.Reset();
	ReservedMove->State = EReservedMoveState::Finished;

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
		if (CurrentOccupiedCount == 0)
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
		ReservedMove->PathTiles.Emplace(MakeWeakObjectPtr(Tile));
	}

	// MoveDistance가 있는 경우 대기 상태로 변경합니다.
	if (const ICombatInterface* Combat = Cast<ICombatInterface>(ReservedMove->PlayerCharacter))
	{
		if (Combat->GetMoveDistance() > 0)
		{
			ReservedMove->State = EReservedMoveState::WaitingForQueue;
		}
	}

	// 이동 예약을 수정했으므로, 턴 종료 버튼 클릭 시 잔여 행동력 여부를 한 번 점검해야 합니다.
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->SetShouldDeferEndPlayerMovePhase();
	}
}

bool UPlayerAbilityContextComponent::AddMoveActivationData()
{
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState)
	{
		return false;
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
	
	bool bIsActivationDataAdded = false;
	bool bHasWaitingForUnblock = false;
	TArray<int32> DelayIndex;
	for (int32 Index = 0; Index < ReservedMoves.Num(); ++Index)
	{
		FPlayerCharacterReservedMove& ReservedMove = ReservedMoves[Index];
		if (!ReservedMove.IsValid() || ReservedMove.State == EReservedMoveState::Finished)
		{
			continue;
		}

		// 남은 이동력은 있으나 나아갈 수 없는 경우, 다른 캐릭터로 인해 경로가 막힌 상태로 간주합니다.
		ATile* NextTile = GetNextReserveTile(&ReservedMove);
		if (!NextTile)
		{
			// 수행 순서를 뒤로 미루기 위해 기록합니다.
			DelayIndex.Emplace(Index);
			ReservedMove.State = EReservedMoveState::WaitingForUnblock;
			bHasWaitingForUnblock = true;
			continue;
		}

		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		ReservedMove.AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			ReservedMove.State = EReservedMoveState::Finished;
			continue;
		}

		// 여기까지 내려온 경우, 이 캐릭터의 이동을 시작합니다.
		ReservedMove.State = EReservedMoveState::Moving;

		FAbilityActivationData AbilityActivationData;
		AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
		AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
		AbilityActivationData.AbilityOwnerASC = ReservedMove.AbilitySystemComponent;
		AbilityActivationData.TargetTile = NextTile;
		LetheGameState->AddPlayerAbilityActivationData(AbilityActivationData, false);
		
		bIsActivationDataAdded = true;
		break;
	}

	// 무한 루프 상태를 방지하기 위해, 이번에 움직일 수 있는 캐릭터가 아무도 없다면 모든 이동 예약을 마무리합니다.
	if (!bIsActivationDataAdded && bHasWaitingForUnblock)
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
	DelayReservedMoves.Reserve(DelayIndex.Num());
	for (int32 Index = DelayIndex.Num() - 1; Index >= 0; --Index)
	{
		const int32 ReservedMoveIndex = DelayIndex[Index];
		DelayReservedMoves.Insert(ReservedMoves[ReservedMoveIndex], 0);
		ReservedMoves.RemoveAt(ReservedMoveIndex);
	}
	ReservedMoves.Append(DelayReservedMoves);

	return bIsActivationDataAdded;
}

ATile* UPlayerAbilityContextComponent::GetNextReserveTile(FPlayerCharacterReservedMove* ReservedMove) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!ReservedMove || !ReservedMove->IsValid() || ReservedMove->PathTiles.IsEmpty() || !TileManagerSubsystem)
	{
		return nullptr;
	}

	if (const ICombatInterface* Combat = Cast<ICombatInterface>(ReservedMove->PlayerCharacter))
	{
		for (int32 Index = Combat->GetMoveDistance() - 1; Index >= 0; --Index)
		{
			const auto& ReservingTile = ReservedMove->PathTiles.IsValidIndex(Index) ? ReservedMove->PathTiles[Index] : nullptr;

			if (ReservingTile.IsValid())
			{
				if (TileManagerSubsystem->CanPlayerMoveToTile(ReservingTile.Get()))
				{
					return ReservingTile.Get();
				}
			}
		}
	}
	return nullptr;
}

void UPlayerAbilityContextComponent::StartResolveMoves()
{
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!LetheGameState || LetheGameState->IsResolvingPlayerAbility())
	{
		return;
	}

	if (AddMoveActivationData())
	{
		LetheGameState->StartActivatePlayerMoveAbilities();
	}
	else
	{
		LetheGameState->TryGoEnemyPlanningPhase();
	}
}

void UPlayerAbilityContextComponent::OnPlayerMoveResolved(const AActor* MovedCharacter)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	FPlayerCharacterReservedMove* ReservedMove = ReservedMoves.FindByPredicate([MovedCharacter](const FPlayerCharacterReservedMove& InReservedMove)
	{
		return InReservedMove.PlayerCharacter == MovedCharacter;
	});
	if (!ReservedMove || !ReservedMove->IsValid())
	{
		return;
	}
	
	if (ReservedMove->IsValid() && ReservedMove->PlayerCharacter == MovedCharacter)
	{
		// 이동 후, 캐싱된 경로에서 도달한 타일까지 제거합니다.
		ATile* CurrentTile = TileManagerSubsystem->GetTileUnderActor(MovedCharacter);
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

	bool bShouldAddActivationData = false;
	for (const FPlayerCharacterReservedMove& RemainReservedMove : ReservedMoves)
	{
		if (RemainReservedMove.State != EReservedMoveState::Finished)
		{
			bShouldAddActivationData = true;
		}
	}

	if (bShouldAddActivationData)
	{
		AddMoveActivationData();
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

				FAbilityActivationData AbilityActivationData;
				AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
				AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
				AbilityActivationData.AbilityOwnerASC = AbilitySystemComponent;
				AbilityActivationData.TargetTile = TargetTile;
				LetheGameState->AddPlayerAbilityActivationData(AbilityActivationData);
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
	AbilityActivationData.TargetTile = OutTileAndActor.Tile;
	LetheGameState->AddPlayerAbilityActivationData(AbilityActivationData);
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
