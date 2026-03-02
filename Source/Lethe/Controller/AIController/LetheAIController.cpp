// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/StateTreeAIComponent.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Manager/LetheGameplayTags.h"

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

void ALetheAIController::SelectRandomAbilityTag()
{
	SelectedAbilityTag = FGameplayTag();

	APawn* ControlledPawn = GetPawn();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(ControlledPawn);
	const UAbilitySystemComponent* ASC = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (ASC)
	{
		TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;
		ASC->GetAllAbilities(AbilitySpecHandles);

		TArray<FGameplayTag> CandidateTags;
		for (const FGameplayAbilitySpecHandle& Handle : AbilitySpecHandles)
		{
			const FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromHandle(Handle);
			if (!Spec || !Spec->Ability)
			{
				continue;
			}

			FGameplayTagContainer AssetTags = Spec->Ability->GetAssetTags();

			if (AssetTags.HasTagExact(FLetheGameplayTags::Get().Ability_Move))
			{
				continue;
			}

			FGameplayTag FirstTag;
			for (const FGameplayTag& Tag : AssetTags)
			{
				FirstTag = Tag;
				break;
			}

			if (FirstTag.IsValid())
			{
				CandidateTags.Emplace(FirstTag);
			}
		}

		if (!CandidateTags.IsEmpty())
		{
			const int32 RandomIndex = FMath::RandRange(0, CandidateTags.Num() - 1);
			SelectedAbilityTag = CandidateTags[RandomIndex];
		}
	}
}

FGameplayTag ALetheAIController::GetSelectedAbilityTag() const
{
	return SelectedAbilityTag;
}
