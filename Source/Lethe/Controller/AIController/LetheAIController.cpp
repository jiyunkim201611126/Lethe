// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/LetheLog.h"
#include "Lethe/AbilitySystem/Ability/LetheGameplayAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"
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
		OnAbilityActivatedDelegateHandle = LetheGameState->OnEnemyAbilityActivated.AddUObject(this, &ThisClass::OnAbilityActivated);
	}
}

void ALetheAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnEnemyAbilityActivated.Remove(OnAbilityActivatedDelegateHandle);
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
	ArrowRenderer->DeactivateCardPreviewArrow();
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
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
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
				[TileManagerSubsystem, &Distance, &OutNearestTiles](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
				{
					if (TileData && TileData->TopTile.IsValid())
					{
						if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(TileData->TopTile.Get()))
						{
							if (ActorOnTile->Implements<UPlayerCharacterInterface>())
							{
								if (OutNearestTiles.IsEmpty() || Distance == Depth)
								{
									Distance = Depth;
									OutNearestTiles.Add(TileData->TopTile.Get());
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

ATile* ALetheAIController::GetBestAttackableTile(const ATile* TargetTile)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	const AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	if (!TargetTile || !TileManagerSubsystem || !ControlledEnemy)
	{
		return nullptr;
	}

	// TargetTile을 공격할 수 있는 위치의 타일을 모두 가져옵니다.
	TArray<ATile*> AttackableTiles;
	const FBFSRange& AbilityRange = ControlledEnemy->GetAbilityRange();

	TSet<FCubeCoord> OutCubeCoord;
	TileManagerSubsystem->TileBFS(TargetTile->GetCubeCoord(), AbilityRange.Distance, AbilityRange.BFSType, OutCubeCoord,
		[](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return true;
		},
		[TileManagerSubsystem, TargetTile, &AbilityRange, &AttackableTiles](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			if (TileData)
			{
				if (ATile* CandidateTile = TileData->TopTile.Get())
				{
					if (CandidateTile != TargetTile)
					{
						const int32 FloorGap = TileManagerSubsystem->GetTileFloor(TargetTile) - TileManagerSubsystem->GetTileFloor(CandidateTile);
						if (FMath::Abs(FloorGap) <= AbilityRange.FloorGap && TileManagerSubsystem->CanEnemyAIMoveToTile(CandidateTile))
						{
							AttackableTiles.Add(CandidateTile);
						}
					}
				}
			}
			return true;
		});

	// 공격 가능한 타일이 아무것도 없다면 nullptr를 반환합니다.
	if (AttackableTiles.IsEmpty())
	{
		return nullptr;
	}

	const auto CalculateDistanceScore = [TileManagerSubsystem, ControlledEnemy](const ATile* CandidateTile)
	{
		if (const ATile* ControlledCharacterTile = TileManagerSubsystem->GetTileUnderActor(ControlledEnemy))
		{
			const int32 Distance = TileManagerSubsystem->GetTileDistance(ControlledCharacterTile, CandidateTile, EBFSType::Connection);
			if (Distance <= ControlledEnemy->GetMoveRange())
			{
				// 이번 턴에 도달 가능한 경우 아주 높은 점수를 반환합니다.
				return 10000;
			}
			// 이번 턴에 도달할 수 없는 경우 멀수록 더 크게 감점합니다.
			return -Distance * 5;
		}
		return -10000;
	};
	
	// TargetTile의 주변 타일을 가져옵니다.
	TArray<ATile*> OutAroundTiles;
	TileManagerSubsystem->GetAroundTiles(TargetTile, 5, OutAroundTiles);

	// 다른 적들과의 타일 좌표상 거리에 따라 점수를 매깁니다.
	const auto CalculateDistanceFromOtherEnemiesScore = [TileManagerSubsystem, OutAroundTiles, ControlledEnemy](const ATile* CandidateTile)
	{
		// Enemy가 서있는 타일만 필터링합니다.
		TArray<ATile*> AroundEnemyTiles = OutAroundTiles.FilterByPredicate([TileManagerSubsystem, ControlledEnemy](const ATile* Tile)
		{
			if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(Tile))
			{
				// ControlledPawn은 제외합니다.
				return ActorOnTile->IsA<AEnemyCharacterBase>() && ActorOnTile != ControlledEnemy;
			}
			return false;
		});
		
		int32 DistanceSumFromOtherEnemies = 0;
		for (const ATile* EnemyTile : AroundEnemyTiles)
		{
			if (!EnemyTile)
			{
				continue;
			}
			
			// 멀수록 더 높은 점수를 갖습니다.
			DistanceSumFromOtherEnemies += FCubeCoord::Distance(CandidateTile->GetCubeCoord(), EnemyTile->GetCubeCoord());
		}
		return DistanceSumFromOtherEnemies;
	};

	// 해당 컨트롤러의 전술 상태에 따라 점수를 매깁니다.(미구현)
	const auto CalculateTacticalScore = [](const ATile* CandidateTile)
	{
		return 0;
	};

	ATile* BestTile = nullptr;
	int32 BestScore = MIN_int32;
	for (ATile* AttackableTile : AttackableTiles)
	{
		if (!AttackableTile)
		{
			continue;
		}

		const int32 DistanceScore = CalculateDistanceScore(AttackableTile);
		const int32 FromOtherEnemiesScore = CalculateDistanceFromOtherEnemiesScore(AttackableTile);

		const int32 Score = DistanceScore + FromOtherEnemiesScore;
		if (BestScore < Score)
		{
			BestScore = Score;
			BestTile = AttackableTile;
		}
	}

	return BestTile;
}

bool ALetheAIController::GetRandomMovePath(const EBFSType BFSType, const int32 MaxDepth, TArray<ATile*>& OutRandomMovePath)
{
	OutRandomMovePath.Reset();
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* StartTile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TSet<FCubeCoord> TilesInRange;
			const FCubeCoord ThisTileCoord = StartTile->GetCubeCoord();
			TileManagerSubsystem->TileBFS(ThisTileCoord, MaxDepth, BFSType, TilesInRange,
				[](const FTileData* CurrentTileData, const FTileData* NextTileData)
				{
					// 우선 범위 내 좌표를 모두 탐색합니다.
					return true;
				},
				[TileManagerSubsystem](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
				{
					// EnemyAI가 이동 가능한 좌표만 선택합니다.
					if (TileData && TileData->TopTile.IsValid())
					{
						return TileManagerSubsystem->CanEnemyAIMoveToTile(TileData->TopTile.Get());
					}
					return false;
				});
			
			if (!TilesInRange.IsEmpty())
			{
				// 범위 내 타일 중 랜덤하게 하나 선택해 경로를 생성합니다.
				TArray<FCubeCoord> TileArray = TilesInRange.Array();
				const FCubeCoord& RandomCoord = TileArray[FMath::RandRange(0, TileArray.Num() - 1)];
				if (const ATile* TargetTile = TileManagerSubsystem->GetTile(RandomCoord))
				{
					GetPrioritizedMoveTiles(TargetTile, MaxDepth, OutRandomMovePath);
					return !OutRandomMovePath.IsEmpty();
				}
			}
		}
	}
	return false;
}

void ALetheAIController::ActivateMoveAbility(const TArray<ATile*>& PathTiles)
{
	if (PathTiles.IsEmpty())
	{
		LETHE_LOG(LogAIController, Error, "PathTiles가 비어있습니다.");
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
			FAbilityActivationContext MoveAbilityActivationContext;
			MoveAbilityActivationContext.AbilitySpecHandle = AbilitySpecs[0]->Handle;
			MoveAbilityActivationContext.AbilityTag = LetheGameplayTags.Ability_Move;
			MoveAbilityActivationContext.AbilityOwnerASC = ASC;
			for (ATile* PathTile : PathTiles)
			{
				if (PathTile)
				{
					MoveAbilityActivationContext.PathTiles.Add(PathTile);
				}
			}
			MoveAbilityActivationContext.Payload.Instigator = ControlledPawn;
			
			if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
			{
				LetheGameState->ActivateAbility(MoveAbilityActivationContext, ETeamSide::Enemy);
			}
		}
	}
}

void ALetheAIController::GetPrioritizedMoveTiles(const ATile* TargetTile, const int32 MoveRange, TArray<ATile*>& OutPathTiles) const
{
	OutPathTiles.Reset();
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* StartTile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			TileManagerSubsystem->FindPrioritizedPathTiles(StartTile, TargetTile, MoveRange, OutPathTiles, false);
		}
	}
}

void ALetheAIController::SelectAndTelegraphRandomAbility(ATile* TargetTile) const
{
	AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledEnemy);
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!ASC || !TileManagerSubsystem)
	{
		return;
	}
	
	AActor* TargetActor = TileManagerSubsystem->GetActorOnTile(TargetTile);
	if (!TargetActor)
	{
		return;
	}
	
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	ASC->GetAllAbilities(AbilitySpecHandles);

	TArray<FAbilityActivationContext> CandidateAbilityContexts;
	CandidateAbilityContexts.Reserve(AbilitySpecHandles.Num());

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

		FAbilityActivationContext& ActivationContext = CandidateAbilityContexts.AddDefaulted_GetRef();
		ActivationContext.Index = ControlledEnemy->GetEnemyAbilityPriority();
		ActivationContext.AbilitySpecHandle = Spec->Handle;
		ActivationContext.AbilityTag = FirstTag;
		ActivationContext.AbilityOwnerASC = ASC;
		ActivationContext.Payload.Instigator = ControlledEnemy;
		
		FTargetSelectionResult& TargetSelectionResult = ActivationContext.TargetSelectionResults.AddDefaulted_GetRef();
		TargetSelectionResult.TargetGroupTag = LetheGameplayTags.TargetGroup_Primary;
		
		FSelectedTarget& Target = TargetSelectionResult.Targets.AddDefaulted_GetRef();
		Target.TargetTile = TargetTile;
		Target.ActorOnTile = TargetActor;
	}

	if (!CandidateAbilityContexts.IsEmpty())
	{
		const int32 RandomIndex = FMath::RandRange(0, CandidateAbilityContexts.Num() - 1);
		if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			LetheGameState->EnqueueEnemyAbilityActivationContext(CandidateAbilityContexts[RandomIndex]);
		}
		
		ArrowRenderer->DrawCardPreviewArrow(ControlledEnemy, { TargetActor });
	}
}

void ALetheAIController::StartCombat()
{
	AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!ControlledEnemy || !TileManagerSubsystem || !RoomManagerSubsystem || !LetheGameState)
	{
		return;
	}
	
	bIsCombating = true;
	
	if (const ATile* CurrentTile = TileManagerSubsystem->GetTileUnderActor(ControlledEnemy))
	{
		RoomManagerSubsystem->RevealEnemyTile(CurrentTile);
		ITileVisionAffectedInterface::Execute_UpdateHiddenByTile(ControlledEnemy, CurrentTile);
	}
	LetheGameState->RegisterCombatEnemy(GetPawn<AEnemyCharacterBase>());
}

bool ALetheAIController::IsCombating() const
{
	return bIsCombating;
}
