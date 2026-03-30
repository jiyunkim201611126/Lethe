// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAbilityContextComponent.h"

#include "ActorSelectorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
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

	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	// 기존에 예약해두었던 타일이 있다면 제거합니다.
	FPlayerCharacterReservedMove* SelectedData = ReservedMoves.FindByPredicate([SelectedCharacter](const FPlayerCharacterReservedMove& MoveData)
	{
		return MoveData.PlayerCharacter == SelectedCharacter;
	});
	if (SelectedData)
	{
		TileManagerSubsystem->RemovePlayerReservedTile(SelectedData->TargetTile.Get());
	}

	// TargetTile을 향한 모든 가능 경로를 가져옵니다.
	TArray<TArray<ATile*>> OutPathTilesArray;
	TileManagerSubsystem->FindShortestPath(TileManagerSubsystem->GetTileUnderActor(SelectedCharacter), TargetTile, OutPathTilesArray, true);
	for (const TArray<ATile*>& PathTiles : OutPathTilesArray)
	{
		// 최단 경로 중, 중간에 가로막히지 않고 도달 가능한 경로를 탐색합니다.
		TArray<TWeakObjectPtr<ATile>> WeakPathTiles;
		bool bIsAllTileEmpty = true;
		for (ATile* Tile : PathTiles)
		{
			if (!TileManagerSubsystem->CanMoveToTileForPlayerCharacter(Tile))
			{
				bIsAllTileEmpty = false;
				break;
			}
			WeakPathTiles.Emplace(Tile);
		}

		if (!bIsAllTileEmpty)
		{
			continue;
		}

		if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(SelectedCharacter))
		{
			// 중간에 가로막히지 않고 도달 가능한 경로라면 이를 캐싱하고, 바로 다음 타일 위치를 예약합니다.
			const int32 MoveDistance = CombatInterface->GetMoveDistance() - 1;
			ATile* ReservingTile = PathTiles.IsValidIndex(MoveDistance) ? PathTiles[MoveDistance] : PathTiles.Last();
			if (!SelectedData)
			{
				FPlayerCharacterReservedMove MovePathData;
				MovePathData.PlayerCharacter = SelectedCharacter;
				MovePathData.AbilitySystemComponent = AbilitySystemComponent;
				MovePathData.PathTiles = WeakPathTiles;
				MovePathData.TargetTile = ReservingTile;
				ReservedMoves.Emplace(MovePathData);
			}
			else
			{
				SelectedData->PathTiles = WeakPathTiles;
				SelectedData->TargetTile = ReservingTile;
			}
			TileManagerSubsystem->ReservePlayerMoveTile(SelectedCharacter, ReservingTile);
			break;
		}
	}
}

void UPlayerAbilityContextComponent::ProcessAllMoves()
{
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!LetheGameState || !TileManagerSubsystem)
	{
		return;
	}

	// 예약된 경로대로 모든 플레이어 캐릭터의 MoveAbility를 발동시킵니다.
	for (FPlayerCharacterReservedMove& ReservedMove : ReservedMoves)
	{
		// 경로가 더이상 남지 않았다면 다음 캐릭터로 넘어갑니다.
		if (!ReservedMove.IsValid())
		{
			continue;
		}

		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
		ReservedMove.AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			return;
		}

		FAbilityActivationData AbilityActivationData;
		AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
		AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
		AbilityActivationData.AbilityOwnerASC = ReservedMove.AbilitySystemComponent;
		AbilityActivationData.TargetTile = ReservedMove.TargetTile;
		LetheGameState->ActivateAbility(AbilityActivationData);

		// 이동 후, 캐싱된 경로에서 도달한 타일까지 제거합니다.
		int32 RemoveNum = 1;
		for (const auto& PathTile : ReservedMove.PathTiles)
		{
			if (PathTile.IsValid() && PathTile == ReservedMove.TargetTile)
			{
				break;
			}
			++RemoveNum;
		}
		for (int32 RemoveIndex = RemoveNum; RemoveIndex > 0; --RemoveIndex)
		{
			if (!ReservedMove.PathTiles.IsEmpty())
			{
				ReservedMove.PathTiles.RemoveAt(0);
			}
		}

		// 다음 이동할 타일을 예약합니다.
		if (!ReservedMove.PathTiles.IsEmpty())
		{
			const ICombatInterface* CombatInterface = Cast<ICombatInterface>(ReservedMove.PlayerCharacter);
			if (!CombatInterface)
			{
				continue;
			}

			const int32 MoveDistance = CombatInterface->GetMoveDistance();
			const auto& ReservingTile = ReservedMove.PathTiles.IsValidIndex(MoveDistance) ? ReservedMove.PathTiles[MoveDistance] : ReservedMove.PathTiles.Last();
			if (ReservingTile.IsValid())
			{
				TileManagerSubsystem->RemovePlayerReservedTile(ReservedMove.TargetTile.Get());
				ReservedMove.TargetTile = ReservingTile;
				TileManagerSubsystem->ReservePlayerMoveTile(ReservedMove.PlayerCharacter.Get(), ReservingTile.Get());
			}
		}
	}

	// 모든 플레이어 캐릭터의 이동을 마쳤다면 다음 페이즈로 넘어갑니다.
	LetheGameState->GoEnemyPlanningPhase();
}

void UPlayerAbilityContextComponent::ResetReservedMoveData()
{
	TArray<AActor*> PlayerCharacters;
	for (const auto& ReservedMove : ReservedMoves)
	{
		if (ReservedMove.PlayerCharacter.IsValid())
		{
			PlayerCharacters.Emplace(ReservedMove.PlayerCharacter.Get());
		}
	}

	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		TileManagerSubsystem->ResetPlayerReservedTile(PlayerCharacters);
	}

	ReservedMoves.Reset();
}

void UPlayerAbilityContextComponent::RequestMove(const AActor* SelectedCharacter, UAbilitySystemComponent* AbilitySystemComponent, const TArray<ATile*>& TilesInRange, ATile* TargetTile) const
{
	if (!SelectedCharacter || !AbilitySystemComponent || !TargetTile)
	{
		return;
	}

	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (TilesInRange.Contains(TargetTile) && TileManagerSubsystem->CanMoveToTileForPlayerCharacter(TargetTile))
		{
			// 선택한 타일로 이동 가능한 경우 들어오는 분기입니다.
			TileManagerSubsystem->ReservePlayerMoveTile(SelectedCharacter, TargetTile);

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
