// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

ALetheAIController::ALetheAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}

void ALetheAIController::SetAbilityPriority(const int32 InPriority)
{
	AbilityPriority = InPriority;
}

void ALetheAIController::BeginPlay()
{
	Super::BeginPlay();

	if (ALetheGameState* LetheGameState = Cast<ALetheGameState>(GetWorld()->GetGameState()))
	{
		LetheGameState->OnChangeTurnStateDelegate.AddUObject(this, &ThisClass::OnPhaseStateChanged);
	}
}

void ALetheAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = Cast<ALetheGameState>(GetWorld()->GetGameState()))
	{
		LetheGameState->OnChangeTurnStateDelegate.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StartLogic();
	}
}

void ALetheAIController::OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase) const
{
	if (!StateTreeAIComponent)
	{
		return;
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	if (NewPhase == EPhaseState::DrawPhase)
	{
		FStateTreeEvent Event;
		Event.Tag = LetheGameplayTags.Event_StateTree_TurnEnded;
		StateTreeAIComponent->SendStateTreeEvent(Event);
	}

	if (NewPhase == EPhaseState::EnemyTurnPhase)
	{
		FStateTreeEvent Event;
		Event.Tag = LetheGameplayTags.Event_StateTree_TurnStarted;
		StateTreeAIComponent->SendStateTreeEvent(Event);
	}
}

int32 ALetheAIController::FindNearestPlayerCharacterTiles(UPARAM(ref) TArray<ATile*>& OutNearestTiles) const
{
	OutNearestTiles.Reset();
	
	UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	const APawn* ControlledPawn = GetPawn();
	if (!TileManagerSubsystem || !ControlledPawn)
	{
		return INDEX_NONE;
	}

	const ATile* ThisTile = TileManagerSubsystem->GetTileUnderActor(ControlledPawn);
	if (!ThisTile)
	{
		return INDEX_NONE;
	}

	TArray<TPair<ATile*, int32>> TileAndDistances;
	TileAndDistances.Reserve(PLAYABLE_CHARACTER_NUMBER);

	int32 ShortestDistance = INT_MAX;
	for (const auto& Elem : TileManagerSubsystem->GetPlayerCharacterToTileMap())
	{
		if (Elem.Value.IsValid())
		{
			const int32 Distance = TileManagerSubsystem->GetTileDistance(ThisTile, Elem.Value.Get(), EBFSType::Connection);
			if (Distance != INDEX_NONE && Distance < ShortestDistance)
			{
				ShortestDistance = Distance;
			}

			if (Distance != INDEX_NONE)
			{
				TileAndDistances.Emplace(Elem.Value.Get(), Distance);
			}
		}
	}

	if (ShortestDistance == INT_MAX)
	{
		return INDEX_NONE;
	}

	OutNearestTiles.Reserve(TileAndDistances.Num());
	for (const TPair<ATile*, int32>& TileAndDistance : TileAndDistances)
	{
		if (TileAndDistance.Value == ShortestDistance)
		{
			OutNearestTiles.Emplace(TileAndDistance.Key);
		}
	}
	
	return ShortestDistance;
}

void ALetheAIController::SelectMoveAbility() const
{
	APawn* ControlledPawn = GetPawn();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
	UAbilitySystemComponent* ASC = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (ASC)
	{
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();

		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
		if (!AbilitySpecs.IsEmpty())
		{
			FAbilityActivationData MoveAbilityActivationData;
			MoveAbilityActivationData.Index = AbilityPriority;
			MoveAbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
			MoveAbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
			MoveAbilityActivationData.AbilityOwnerASC = ASC;
			MoveAbilityActivationData.Payload.Instigator = ControlledPawn;
			
			if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
			{
				LetheGameState->AddEnemyAbilityActivationData(MoveAbilityActivationData);
			}
		}
	}
}

void ALetheAIController::SelectRandomAbility() const
{
	APawn* ControlledPawn = GetPawn();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
	UAbilitySystemComponent* ASC = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (ASC)
	{
		TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
		ASC->GetAllAbilities(AbilitySpecHandles);

		TArray<FAbilityActivationData> CandidateAbilityData;
		CandidateAbilityData.Reserve(AbilitySpecHandles.Num());

		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
		{
			const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
			if (!Spec || !Spec->Ability)
			{
				continue;
			}

			const FGameplayTagContainer AssetTags = Spec->Ability->GetAssetTags();
			if (AssetTags.HasTagExact(LetheGameplayTags.Ability_Move))
			{
				continue;
			}

			FGameplayTag FirstTag;
			for (const FGameplayTag& Tag : AssetTags)
			{
				if (Tag.IsValid())
				{
					FirstTag = Tag;
					break;
				}
			}

			FAbilityActivationData ActivationData;
			ActivationData.Index = AbilityPriority;
			ActivationData.AbilitySpecHandle = Spec->Handle;
			ActivationData.AbilityTag = FirstTag;
			ActivationData.AbilityOwnerASC = ASC;
			ActivationData.Payload.Instigator = ControlledPawn;

			CandidateAbilityData.Emplace(ActivationData);
		}

		if (!CandidateAbilityData.IsEmpty())
		{
			const int32 RandomIndex = FMath::RandRange(0, CandidateAbilityData.Num() - 1);
			if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
			{
				LetheGameState->AddEnemyAbilityActivationData(CandidateAbilityData[RandomIndex]);
			}
		}
	}
}

void ALetheAIController::SetTargetTile(ATile* TargetTile) const
{
	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->SetTargetTileForEnemy(AbilityPriority, TargetTile);
	}
}

void ALetheAIController::SetTargetTileToMove(ATile* CurrentTile, ATile* TargetTile)
{
	if (TargetTile)
	{
		if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			TileManagerSubsystem->RemoveToReservedMoveTiles(CurrentTile);
			TileManagerSubsystem->AddToReservedMoveTiles(TargetTile);
		}
		SetTargetTile(TargetTile);
	}
}

TArray<ATile*> ALetheAIController::GetPathTiles(ATile* TargetTile) const
{
	TArray<ATile*> PathTiles;
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* ThisTile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TArray<TArray<ATile*>> PathTilesArray;
			if (TileManagerSubsystem->FindShortestPath(ThisTile, TargetTile, PathTilesArray) && !PathTilesArray.IsEmpty())
			{
				PathTiles = PathTilesArray[0];
			}
		}
	}
	return PathTiles;
}

TArray<FTilePath> ALetheAIController::GetAllPathTiles(ATile* TargetTile) const
{
	TArray<FTilePath> OutPathTiles;
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* ThisTile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TArray<TArray<ATile*>> PathTilesArray;
			if (TileManagerSubsystem->FindShortestPath(ThisTile, TargetTile, PathTilesArray))
			{
				OutPathTiles.Reserve(PathTilesArray.Num());
				for (const TArray<ATile*>& PathTiles : PathTilesArray)
				{
					FTilePath TilePath;
					TilePath.Tiles = PathTiles;
					OutPathTiles.Emplace(MoveTemp(TilePath));
				}
			}
		}
	}

	return OutPathTiles;
}
