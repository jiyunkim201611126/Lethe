// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
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
		LetheGameState->OnActivateEnemyAbilityDelegate.AddUObject(this, &ThisClass::OnAbilityActivated);
	}

	check(ArrowRendererClass);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ArrowRenderer = GetWorld()->SpawnActor<AArrowRenderer>(ArrowRendererClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
}

void ALetheAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = Cast<ALetheGameState>(GetWorld()->GetGameState()))
	{
		LetheGameState->OnChangeTurnStateDelegate.RemoveAll(this);
		LetheGameState->OnActivateEnemyAbilityDelegate.RemoveAll(this);
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

void ALetheAIController::OnAbilityActivated(AActor* AbilityInstigator) const
{
	if (AbilityInstigator == GetPawn())
	{
		ArrowRenderer->SetActive(false);
	}
}

int32 ALetheAIController::FindNearestPlayerCharacterTiles(const int32 MaxDepth, const EBFSType BFSType, TArray<ATile*>& OutNearestTiles)
{
	int32 Distance = INDEX_NONE;
	OutNearestTiles.Reset();
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* Tile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			const FCubeCoord ThisTileCoord = Tile->GetCubeCoord();
			TSet<FCubeCoord> PlayerCharacterTileCoords;
			TileManagerSubsystem->TileBFS(ThisTileCoord, MaxDepth, BFSType, PlayerCharacterTileCoords,
			[&PlayerCharacterTileCoords](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return PlayerCharacterTileCoords.IsEmpty();
			},
			[&TileManagerSubsystem, &Distance, &OutNearestTiles](const FCubeCoord CurrentCoord, const FTileData* TileData, const int32 Depth)
			{
				if (TileData && TileData->TileActor.IsValid())
				{
					if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(TileData->TileActor.Get()))
					{
						if (ActorOnTile->Implements<UPlayableCharacterInterface>())
						{
							if (OutNearestTiles.IsEmpty() || Distance == Depth)
							{
								Distance = Depth;
								OutNearestTiles.Emplace(TileData->TileActor.Get());
								return true;
							}
							if (!OutNearestTiles.IsEmpty() && Distance != Depth)
							{
								return false;
							}
						}
					}
				}
				return false;
			});
		}
	}
	return Distance;
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

void ALetheAIController::GetPrioritizedMoveTiles(ATile* TargetTile, TArray<ATile*>& OutPathTiles) const
{
	OutPathTiles.Reset();
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* ThisTile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TArray<TArray<ATile*>> PathTilesArray;
			if (TileManagerSubsystem->FindShortestPath(ThisTile, TargetTile, PathTilesArray))
			{
				for (int32 Index = MoveLength - 1; Index >= 0; --Index)
				{
					for (TArray<ATile*>& PathTiles : PathTilesArray)
					{
						if (PathTiles.IsValidIndex(Index))
						{
							OutPathTiles.Emplace(PathTiles[Index]);
						}
					}
				}
			}
		}
	}
}

void ALetheAIController::SetTargetTile(ATile* TargetTile) const
{
	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->SetTargetTileForEnemy(AbilityPriority, TargetTile);
		if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			ArrowRenderer->SetPoints(GetPawn(), TileManagerSubsystem->GetActorOnTile(TargetTile));
		}
	}
}

void ALetheAIController::SetTargetTileToMove(ATile* CurrentTile, ATile* TargetTile)
{
	if (TargetTile)
	{
		if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			TileManagerSubsystem->RemoveToStandingOrReservedMoveTiles(CurrentTile);
			TileManagerSubsystem->AddToStandingOrReservedMoveTiles(TargetTile);
		}
		if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			LetheGameState->SetTargetTileForEnemy(AbilityPriority, TargetTile);
		}
		ArrowRenderer->SetPoints(GetPawn(), TargetTile, false);
	}
}
