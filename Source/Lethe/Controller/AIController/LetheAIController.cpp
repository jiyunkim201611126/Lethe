// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/LetheLog.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/AbilitySystem/Ability/LetheGameplayAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
#include "Lethe/Controller/PlayerController/ActorSelectorComponent.h"
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
	ActorSelector = CreateDefaultSubobject<UActorSelectorComponent>(TEXT("ActorSelector"));
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
		OnEnemyAbilityTriedActivateDelegateHandle = LetheGameState->OnEnemyAbilityTriedActivate.AddUObject(this, &ThisClass::OnEnemyAbilityTriedActivate);
	}
}

void ALetheAIController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnEnemyAbilityTriedActivate.Remove(OnEnemyAbilityTriedActivateDelegateHandle);
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

void ALetheAIController::ProcessCommitPlan() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	FStateTreeEvent Event;
	Event.Tag = LetheGameplayTags.Event_StateTree_CommitPlan;
	StateTreeAIComponent->SendStateTreeEvent(Event);
}

void ALetheAIController::DeactivateArrow() const
{
	ArrowRenderer->DeactivateCardPreviewArrow();
}

void ALetheAIController::OnEnemyAbilityTriedActivate(AActor* AbilityInstigator) const
{
	if (AbilityInstigator == GetPawn())
	{
		DeactivateArrow();
		ActorSelector->ClearHighlightedActors(EHighlightReason::TargetedByAI);
	}
}

int32 ALetheAIController::FindNearestPlayerCharacterTiles(const EBFSType BFSType, const int32 MaxDepth, TArray<ATile*>& OutTiles)
{
	int32 Distance = INDEX_NONE;
	OutTiles.Reset();
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
				[TileManagerSubsystem, &Distance, &OutTiles](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
				{
					if (TileData && TileData->TopTile.IsValid())
					{
						if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(TileData->TopTile.Get()))
						{
							if (ActorOnTile->Implements<UPlayerCharacterInterface>())
							{
								if (OutTiles.IsEmpty() || Distance == Depth)
								{
									Distance = Depth;
									OutTiles.Add(TileData->TopTile.Get());
									return true;
								}
								if (!OutTiles.IsEmpty() && Distance != Depth)
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

bool ALetheAIController::GetRandomAbility(FGameplayAbilitySpecHandle& OutAbilitySpecHandle)
{
	APawn* ControlledEnemy = GetPawn();
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledEnemy);
	if (!ASC)
	{
		return false;
	}

	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
	ASC->GetAllAbilities(AbilitySpecHandles);

	TArray<const FGameplayAbilitySpecHandle> CandidateAbilitySpecHandles;
	for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
	{
		const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
		if (!Spec)
		{
			continue;
		}

		if (Cast<ULetheCardAbility>(Spec->Ability))
		{
			CandidateAbilitySpecHandles.Add(Handle);
		}
	}

	if (CandidateAbilitySpecHandles.IsEmpty())
	{
		return false;
	}

	const int32 RandomIndex = FMath::RandRange(0, CandidateAbilitySpecHandles.Num() - 1);
	OutAbilitySpecHandle = CandidateAbilitySpecHandles[RandomIndex];
	return OutAbilitySpecHandle.IsValid();
}

bool ALetheAIController::GetAttackOriginTiles(const FGameplayAbilitySpecHandle AbilitySpecHandle, const ATile* TargetTile, TArray<ATile*>& OutAttackOriginTiles)
{
	OutAttackOriginTiles.Reset();

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	APawn* ControlledEnemy = GetPawn();
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledEnemy);
	if (!AbilitySpecHandle.IsValid() || !TargetTile || !TileManagerSubsystem || !ControlledEnemy || !ASC)
	{
		return false;
	}

	const FGameplayAbilitySpec* AbilitySpec = ASC->FindAbilitySpecFromHandle(AbilitySpecHandle);
	const ULetheCardAbility* CardAbility = AbilitySpec ? Cast<ULetheCardAbility>(AbilitySpec->Ability) : nullptr;
	ATile* SourceTile = TileManagerSubsystem->GetTileUnderActor(ControlledEnemy);
	const AActor* TargetActor = TileManagerSubsystem->GetActorOnTile(TargetTile);
	if (!CardAbility || !SourceTile || !TargetActor)
	{
		return false;
	}

	// 기본적으로 타일 A에서 타일 B를 어떤 Ability로 공격 가능하다면, 타일 B에서 타일 A를 동일한 Ability로 공격할 수 있습니다.
	// 그 점을 이용해, TargetActor가 서있는 타일을 기준으로 GetCandidateTiles를 호출, 공격할 수 있는 타일을 가져옵니다.
	FTargetingIntent TargetingIntent;
	TargetingIntent.HitTile = SourceTile;
	TargetingIntent.ImpactPoint = SourceTile->GetActorLocation();

	FEffectTargetTileSelectorContext Context;
	Context.AvatarActor = TargetActor;
	Context.SourceTile = TargetTile;
	Context.TargetingIntent = TargetingIntent;

	FEffectTargetTileSelectorResult Result;
	CardAbility->GetCandidateTiles(Context, Result);

	for (ATile* CandidateTile : Result.OutSelectCandidateTiles)
	{
		if (!CandidateTile || CandidateTile == TargetTile)
		{
			continue;
		}

		if (CandidateTile == SourceTile || TileManagerSubsystem->CanEnemyAIMoveToTile(CandidateTile))
		{
			OutAttackOriginTiles.AddUnique(CandidateTile);
		}
	}

	return !OutAttackOriginTiles.IsEmpty();
}

ATile* ALetheAIController::SelectBestAttackOriginTile(const TArray<ATile*>& AttackOriginTiles)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	const AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	if (!TileManagerSubsystem || !ControlledEnemy || AttackOriginTiles.IsEmpty())
	{
		return nullptr;
	}

	const auto CalculateDistanceScore = [TileManagerSubsystem, ControlledEnemy](const ATile* CandidateTile)
	{
		if (const ATile* ControlledCharacterTile = TileManagerSubsystem->GetTileUnderActor(ControlledEnemy))
		{
			const int32 Distance = TileManagerSubsystem->GetTileDistance(ControlledCharacterTile, CandidateTile, EBFSType::Connection);
			if (Distance == INDEX_NONE)
			{
				return -10000;
			}
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

	// 해당 컨트롤러의 전술 상태에 따라 점수를 매깁니다.(미구현)
	const auto CalculateTacticalScore = [](const ATile* CandidateTile)
	{
		return 0;
	};

	ATile* BestTile = nullptr;
	int32 BestScore = MIN_int32;
	for (ATile* AttackableTile : AttackOriginTiles)
	{
		if (!AttackableTile)
		{
			continue;
		}

		const int32 DistanceScore = CalculateDistanceScore(AttackableTile);
		//const int32 TacticalScore = CalculateTacticalScore(AttackableTile);

		const int32 Score = DistanceScore;// + TacticalScore;
		if (BestScore < Score)
		{
			BestScore = Score;
			BestTile = AttackableTile;
		}
	}

	return BestTile;
}

void ALetheAIController::SetPlannedData(const FGameplayAbilitySpecHandle SelectedAbilitySpecHandle, ATile* TargetTile)
{
	PlannedAbilitySpecHandle = SelectedAbilitySpecHandle;
	PlannedTargetTile = TargetTile;
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

void ALetheAIController::CommitPlan()
{
	AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledEnemy);
	if (!PlannedAbilitySpecHandle.IsValid() || !PlannedTargetTile.IsValid() || !ControlledEnemy || !ASC)
	{
		return;
	}

	const FGameplayAbilitySpec* SelectedAbilitySpec = ASC->FindAbilitySpecFromHandle(PlannedAbilitySpecHandle);
	const ULetheCardAbility* SelectedCardAbility = SelectedAbilitySpec ? Cast<ULetheCardAbility>(SelectedAbilitySpec->Ability) : nullptr;
	if (!SelectedCardAbility)
	{
		return;
	}

	FGameplayTag AbilityTag = FGameplayTag::EmptyTag;
	const FGameplayTagContainer AssetTags = SelectedCardAbility->GetAssetTags();
	const FGameplayTag CardRootTag = FGameplayTag::RequestGameplayTag(TEXT("Card"));
	for (const FGameplayTag& Tag : AssetTags)
	{
		if (Tag.MatchesTag(CardRootTag))
		{
			AbilityTag = Tag;
			break;
		}
	}

	FTargetingIntent TargetingIntent;
	TargetingIntent.HitTile = PlannedTargetTile.Get();
	TargetingIntent.ImpactPoint = PlannedTargetTile->GetActorLocation();

	FEffectTargetTileSelectorContext ContextForTileHighlight;
	ContextForTileHighlight.AvatarActor = ControlledEnemy;
	ContextForTileHighlight.TargetingIntent = TargetingIntent;

	FEffectTargetTileSelectorResult ResultForTileHighlight;
	SelectedCardAbility->GetCandidateTiles(ContextForTileHighlight, ResultForTileHighlight);
	
	TArray<ATile*> HighlightTiles;
	for (const FTargetSelectionResult& TargetResult : ResultForTileHighlight.OutTargetCandidates)
	{
		HighlightTiles.Append(TargetResult.GetTargetTiles());
	}
	ActorSelector->SetHighlightedTiles(EHighlightReason::TargetedByAI, HighlightTiles);

	FEffectTargetTileSelectorContext Context;
	Context.AvatarActor = ControlledEnemy;
	Context.TargetingIntent = TargetingIntent;

	FEffectTargetTileSelectorResult Result;
	SelectedCardAbility->GetTargetTilesForAI(Context, Result);

	FAbilityActivationContext ActivationContext;
	ActivationContext.Index = ControlledEnemy->GetEnemyAbilityPriority();
	ActivationContext.AbilitySpecHandle = PlannedAbilitySpecHandle;
	ActivationContext.TargetingIntent = Context.TargetingIntent;
	ActivationContext.AbilityTag = AbilityTag;
	ActivationContext.AbilityOwnerASC = ASC;
	ActivationContext.TargetSelectionResults = MoveTemp(Result.OutTargetResults);
	ActivationContext.Payload.Instigator = ControlledEnemy;
	
	PlannedAbilitySpecHandle = FGameplayAbilitySpecHandle();
	PlannedTargetTile.Reset();
	
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->EnqueueEnemyAbilityActivationContext(ActivationContext);
	}
}

void ALetheAIController::StartCombat()
{
	AEnemyCharacterBase* ControlledEnemy = GetPawn<AEnemyCharacterBase>();
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>();
	const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
	if (!ControlledEnemy || !TileManagerSubsystem || !RoomManagerSubsystem || !LetheGameState)
	{
		return;
	}
	
	bIsInCombat = true;
	
	if (const ATile* SourceTile = TileManagerSubsystem->GetTileUnderActor(ControlledEnemy))
	{
		RoomManagerSubsystem->RevealEnemyTile(SourceTile);
		ITileVisionAffectedInterface::Execute_UpdateHiddenByTile(ControlledEnemy, SourceTile);
	}
	LetheGameState->RegisterCombatEnemy(GetPawn<AEnemyCharacterBase>());
}

bool ALetheAIController::IsInCombat() const
{
	return bIsInCombat;
}
