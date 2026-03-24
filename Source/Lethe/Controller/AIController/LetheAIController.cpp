// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ALetheAIController::ALetheAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
}

void ALetheAIController::BeginPlay()
{
	Super::BeginPlay();

	check(ArrowRendererClass);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ArrowRenderer = GetWorld()->SpawnActor<AArrowRenderer>(ArrowRendererClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);

	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnEnemyAbilityActivated.AddUObject(this, &ThisClass::OnAbilityActivated);
	}
}

void ALetheAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnEnemyAbilityActivated.RemoveAll(this);
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

void ALetheAIController::OnUnPossess()
{
	if (StateTreeAIComponent)
	{
		StateTreeAIComponent->StopLogic(FString(""));
	}
	
	Super::OnUnPossess();
}

void ALetheAIController::ProcessPlanPhase() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	FStateTreeEvent Event;
	Event.Tag = LetheGameplayTags.Event_StateTree_PlanPhaseStarted;
	StateTreeAIComponent->SendStateTreeEvent(Event);
}

void ALetheAIController::ProcessTelegraphPlan() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	FStateTreeEvent Event;
	Event.Tag = LetheGameplayTags.Event_StateTree_TelegraphPlan;
	StateTreeAIComponent->SendStateTreeEvent(Event);
}

void ALetheAIController::DeactivateArrow() const
{
	ArrowRenderer->SetActive(false);
}

void ALetheAIController::OnAbilityActivated(AActor* AbilityInstigator) const
{
	if (AbilityInstigator == GetPawn())
	{
		DeactivateArrow();
	}
}

int32 ALetheAIController::FindNearestPlayerCharacterTiles(const EBFSType BFSType, const int32 MaxDepth, TArray<ATile*>& OutNearestTiles)
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

ATile* ALetheAIController::GetRandomMovableTile(const EBFSType BFSType, const int32 MaxDepth)
{
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* Tile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TSet<FCubeCoord> TilesInRange;
			const FCubeCoord ThisTileCoord = Tile->GetCubeCoord();
			TileManagerSubsystem->TileBFS(ThisTileCoord, MaxDepth, BFSType, TilesInRange,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				// 우선 범위 내 타일을 모두 탐색합니다.
				return true;
			},
			[&TileManagerSubsystem](const FCubeCoord CurrentCoord, const FTileData* TileData, const int32 Depth)
			{
				// EnemyAI가 이동 가능한 타일만 선택합니다.
				if (TileData && TileData->TileActor.IsValid())
				{
					return TileManagerSubsystem->CanMoveToTileForEnemyAI(TileData->TileActor.Get());
				}
				return false;
			});
		
			if (!TilesInRange.IsEmpty())
			{
				// 범위 내 타일 중 랜덤하게 하나 반환합니다.
				TArray<FCubeCoord> TileArray = TilesInRange.Array();
				const FCubeCoord& RandomCoord = TileArray[FMath::RandRange(0, TileArray.Num() - 1)];
				return TileManagerSubsystem->GetTile(RandomCoord);
			}
		}
	}
	return nullptr;
}

void ALetheAIController::ActivateMoveAbility(ATile* TargetTile)
{
	if (!TargetTile)
	{
		return;
	}
	
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
			MoveAbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
			MoveAbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
			MoveAbilityActivationData.AbilityOwnerASC = ASC;
			MoveAbilityActivationData.TargetTile = TargetTile;
			MoveAbilityActivationData.Payload.Instigator = ControlledPawn;
			
			if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
			{
				LetheGameState->ActivateEnemyAbility(MoveAbilityActivationData);
			}
		}
	}
}

void ALetheAIController::GetPrioritizedMoveTiles(const ATile* TargetTile, const int32 MoveDistance, TArray<ATile*>& OutPathTiles) const
{
	OutPathTiles.Reset();
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* ThisTile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TileManagerSubsystem->FindPrioritizedPathTilesForAI(ThisTile, TargetTile, MoveDistance, OutPathTiles);
		}
	}
}

void ALetheAIController::SelectAndTelegraphRandomAbility(ATile* TargetTile) const
{
	const AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(ControlledEnemy);
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
			ActivationData.Index = ControlledEnemy->GetEnemyAbilityPriority();
			ActivationData.AbilitySpecHandle = Spec->Handle;
			ActivationData.AbilityTag = FirstTag;
			ActivationData.AbilityOwnerASC = ASC;
			ActivationData.TargetTile = TargetTile;
			ActivationData.Payload.Instigator = ControlledEnemy;

			CandidateAbilityData.Emplace(ActivationData);
		}

		if (!CandidateAbilityData.IsEmpty())
		{
			const int32 RandomIndex = FMath::RandRange(0, CandidateAbilityData.Num() - 1);
			if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
			{
				LetheGameState->AddEnemyAbilityActivationData(CandidateAbilityData[RandomIndex]);
			}
			
			if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
			{
				ArrowRenderer->SetPoints(GetPawn(), TileManagerSubsystem->GetActorOnTile(TargetTile));
			}
		}
	}
}

bool ALetheAIController::IsPlayerCharacterInDetectionRange()
{
	if (const AEnemyCharacterBase* EnemyCharacter = GetPawn<AEnemyCharacterBase>())
	{
		const FBFSRange& AbilityRange = EnemyCharacter->GetAbilityRange();
		TArray<ATile*> PlayerCharacterTiles;
		FindNearestPlayerCharacterTiles(AbilityRange.BFSType, AbilityRange.Distance, PlayerCharacterTiles);
		return !PlayerCharacterTiles.IsEmpty();
	}
	return false;
}
