// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAbilityContextComponent.h"

#include "ActorSelectorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
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
		if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(SelectedCharacter))
		{
			FBFSRange MoveRange;
			MoveRange.BFSType = EBFSType::Connection;
			MoveRange.Distance = CombatInterface->GetMoveDistance();
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
	FPlayerCharacterReservedMove* MovePathData = ReservedMoves.FindByPredicate([SelectedCharacter](const FPlayerCharacterReservedMove& MoveData)
	{
		return MoveData.PlayerCharacter == SelectedCharacter;
	});
	if (!MovePathData)
	{
		MovePathData = &ReservedMoves.Emplace_GetRef();
		MovePathData->PlayerCharacter = SelectedCharacter;
		MovePathData->AbilitySystemComponent = AbilitySystemComponent;
	}

	// TargetTile을 향한 모든 가능 경로를 가져옵니다.
	TArray<TArray<ATile*>> OutPathTilesArray;
	TileManagerSubsystem->FindShortestPath(TileManagerSubsystem->GetTileUnderActor(SelectedCharacter), TargetTile, OutPathTilesArray, true);
	for (const TArray<ATile*>& PathTiles : OutPathTilesArray)
	{
		TArray<TWeakObjectPtr<ATile>> WeakPathTiles;
		for (ATile* Tile : PathTiles)
		{
			WeakPathTiles.Emplace(Tile);
		}
		MovePathData->PathTiles = WeakPathTiles;

		if (ReserveNextMoveTile(MovePathData, true))
		{
			break;
		}
	}

	// 이동 예약 입력이 수행됐으므로, 턴 종료 입력 시 행동력 잔여 여부를 확인하고 턴을 종료하도록 플래그를 수정합니다.
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->InvalidateMoveTurnEndConfirmation();
	}
}

void UPlayerAbilityContextComponent::ProcessAllMoves()
{
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!LetheGameState || !TileManagerSubsystem)
	{
		return;
	}

	// 캐릭터 인덱스 순서대로 이동 예약을 정렬합니다.
	ReservedMoves.Sort([](const FPlayerCharacterReservedMove& MoveDataA, const FPlayerCharacterReservedMove& MoveDataB)
	{
		const IPlayerCharacterInterface* PlayerCharacterA = CastChecked<IPlayerCharacterInterface>(MoveDataA.PlayerCharacter);
		const IPlayerCharacterInterface* PlayerCharacterB = CastChecked<IPlayerCharacterInterface>(MoveDataB.PlayerCharacter);
		if (PlayerCharacterA && PlayerCharacterB)
		{
			return PlayerCharacterA->GetPlayerOrderIndex() < PlayerCharacterB->GetPlayerOrderIndex();
		}
		return false;
	});

	// 예약된 경로대로 모든 플레이어 캐릭터의 MoveAbility를 발동시킵니다.
	for (int32 Index = 0; Index < ReservedMoves.Num(); ++Index)
	{
		FPlayerCharacterReservedMove& ReservedMove = ReservedMoves[Index];
		if (!ReservedMove.IsValid())
		{
			continue;
		}

		// 경로가 더이상 남지 않았다면 다음 캐릭터로 넘어갑니다.
		if (ReservedMove.PathTiles.IsEmpty())
		{
			continue;
		}

		// 경로가 막혀 나아갈 수 없는 상태였다면 다시 한 번 확인해보고, 그래도 안 된다면 다음 캐릭터로 넘어갑니다.
		if (!ReservedMove.TargetTile.IsValid())
		{
			if (!ReserveNextMoveTile(&ReservedMove, true))
			{
				continue;
			}
		}

		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
		ReservedMove.AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			continue;
		}

		FAbilityActivationData AbilityActivationData;
		AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
		AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
		AbilityActivationData.AbilityOwnerASC = ReservedMove.AbilitySystemComponent;
		AbilityActivationData.TargetTile = ReservedMove.TargetTile;
		LetheGameState->AddPlayerAbilityActivationData(AbilityActivationData, false);
	}
	LetheGameState->StartActivatePlayerAbility();
}

void UPlayerAbilityContextComponent::OnPlayerMoveResolved(const AActor* MovedCharacter)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	FPlayerCharacterReservedMove* SelectedData = ReservedMoves.FindByPredicate([MovedCharacter](const FPlayerCharacterReservedMove& MoveData)
	{
		return MoveData.PlayerCharacter == MovedCharacter;
	});
	if (!SelectedData || !SelectedData->IsValid())
	{
		return;
	}
	
	if (SelectedData->IsValid() && SelectedData->PlayerCharacter == MovedCharacter)
	{
		// 이동 후, 캐싱된 경로에서 도달한 타일까지 제거합니다.
		ATile* CurrentTile = TileManagerSubsystem->GetTileUnderActor(MovedCharacter);
		if (!CurrentTile)
		{
			return;
		}

		const int32 Index = SelectedData->PathTiles.IndexOfByKey(CurrentTile);
		if (Index != INDEX_NONE)
		{
			SelectedData->PathTiles.RemoveAt(0, Index + 1);
		}
		
		// 다음 이동할 타일을 예약합니다.
		ReserveNextMoveTile(SelectedData, false);
	}
}

bool UPlayerAbilityContextComponent::ReserveNextMoveTile(FPlayerCharacterReservedMove* SelectedData, const bool bUseCurrentMoveDistance) const
{
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!SelectedData || !SelectedData->IsValid() || SelectedData->PathTiles.IsEmpty() || !TileManagerSubsystem)
	{
		return false;
	}
	
	const ICombatInterface* CombatInterface = Cast<ICombatInterface>(SelectedData->PlayerCharacter);
	if (!CombatInterface)
	{
		return false;
	}

	const int32 MaxMoveDistance = bUseCurrentMoveDistance ? CombatInterface->GetMoveDistance() - 1 : CombatInterface->GetMaxMoveDistance() - 1;
	ATile* ReservingTile = GetNextReserveTile(SelectedData, MaxMoveDistance);

	if (ReservingTile)
	{
		SelectedData->TargetTile = ReservingTile;
		TileManagerSubsystem->OccupyPlayerMoveTile(SelectedData->PlayerCharacter.Get(), ReservingTile);
		return true;
	}
	
	// 예약된 경로로 나아갈 수 없는 상태라면, TargetTile을 비우고 점유 상태를 초기화합니다.
	SelectedData->TargetTile = nullptr;
	ATile* CurrentTile = TileManagerSubsystem->GetTileUnderActor(SelectedData->PlayerCharacter.Get());
	TileManagerSubsystem->OccupyPlayerMoveTile(SelectedData->PlayerCharacter.Get(), CurrentTile);
	return false;
}

ATile* UPlayerAbilityContextComponent::GetNextReserveTile(FPlayerCharacterReservedMove* SelectedData, const int32 MoveDistance) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!SelectedData || !SelectedData->IsValid() || SelectedData->PathTiles.IsEmpty() || !TileManagerSubsystem)
	{
		return nullptr;
	}
	
	for (int32 Index = MoveDistance; Index >= 0; --Index)
	{
		const auto& ReservingTile = SelectedData->PathTiles.IsValidIndex(Index) ? SelectedData->PathTiles[Index] : nullptr;

		if (ReservingTile.IsValid())
		{
			// 해당 타일로 이동 가능하거나, 애초에 현재 캐릭터가 점유한 타일이었다면 해당 타일을 반환합니다.
			const bool bCanMove = TileManagerSubsystem->CanPlayerMoveToTile(ReservingTile.Get());
			const bool bIsCurrentCharacterReservedTile = SelectedData && SelectedData->TargetTile == ReservingTile;
			if (bCanMove || bIsCurrentCharacterReservedTile)
			{
				return ReservingTile.Get();
			}
		}
	}
	return nullptr;
}

void UPlayerAbilityContextComponent::ResetReservedMoveData()
{
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		TileManagerSubsystem->ResetPlayerOccupiedTile();
	}

	ReservedMoves.Reset();
}

void UPlayerAbilityContextComponent::RequestMove(AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const
{
	if (!SelectedCharacter || !AbilitySystemComponent || !TargetTile)
	{
		return;
	}

	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (TilesInRange.Contains(TargetTile) && TileManagerSubsystem->CanPlayerMoveToTile(TargetTile))
		{
			// 선택한 타일로 이동 가능한 경우 들어오는 분기입니다.
			TileManagerSubsystem->OccupyPlayerMoveTile(SelectedCharacter, TargetTile);

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
