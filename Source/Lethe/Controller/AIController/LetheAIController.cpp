// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

ALetheAIController::ALetheAIController()
{
	PrimaryActorTick.bCanEverTick = false;

	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTreeAIComponent"));
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

void ALetheAIController::SelectMoveAbility()
{
	SelectedAbilityData = FAbilityActivationData();

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
			SelectedAbilityData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
			SelectedAbilityData.AbilityTag = LetheGameplayTags.Ability_Move;
			SelectedAbilityData.AbilityOwnerASC = ASC;
			SelectedAbilityData.Payload.Instigator = ControlledPawn;
		}
	}
}

void ALetheAIController::SelectRandomAbility()
{
	SelectedAbilityData = FAbilityActivationData();

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
			ActivationData.AbilitySpecHandle = Spec->Handle;
			ActivationData.AbilityTag = FirstTag;
			ActivationData.AbilityOwnerASC = ASC;
			ActivationData.Payload.Instigator = ControlledPawn;

			CandidateAbilityData.Emplace(ActivationData);
		}

		if (!CandidateAbilityData.IsEmpty())
		{
			const int32 RandomIndex = FMath::RandRange(0, CandidateAbilityData.Num() - 1);
			SelectedAbilityData = CandidateAbilityData[RandomIndex];
		}
	}
}

void ALetheAIController::FindNearestPlayerCharacterTileCoord()
{
	TargetTile.Reset();
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* Tile = TileManagerSubsystem->GetTileUnderActor(GetPawn()))
		{
			const FCubeCoord ThisTileCoord = Tile->GetCubeCoord();
			TSet<FCubeCoord> PlayerCharacterTileCoords;
			TileManagerSubsystem->TileBFS(ThisTileCoord, 999, EBFSType::Connection, PlayerCharacterTileCoords,
			[&PlayerCharacterTileCoords](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return PlayerCharacterTileCoords.IsEmpty();
			},
			[&TileManagerSubsystem, this](const FCubeCoord CurrentCoord, const FTileData* TileData, int32 Depth)
			{
				if (TileData && TileData->TileActor.IsValid())
				{
					if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(TileData->TileActor.Get()))
					{
						if (ActorOnTile->Implements<UPlayableCharacterInterface>())
						{
							this->TargetTile = TileData->TileActor;
							return true;
						}
					}
				}
				return false;
			});
		}
	}
}

bool ALetheAIController::UseAbilityToTargetTile()
{
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (TargetTile.IsValid())
		{
			AActor* TargetActor = TileManagerSubsystem->GetActorOnTile(TargetTile.Get());
			SelectedAbilityData.Payload.Target = TargetActor;
			return SelectedAbilityData.AbilityOwnerASC->TriggerAbilityFromGameplayEvent(SelectedAbilityData.AbilitySpecHandle, SelectedAbilityData.AbilityOwnerASC->AbilityActorInfo.Get(), SelectedAbilityData.AbilityTag, &SelectedAbilityData.Payload, *SelectedAbilityData.AbilityOwnerASC);
		}
	}
	return false;
}
