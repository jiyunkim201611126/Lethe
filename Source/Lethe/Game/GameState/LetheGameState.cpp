// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

ALetheGameState::ALetheGameState()
{
	AbilityResolverComponent = CreateDefaultSubobject<UAbilityResolverComponent>("AbilityResolverComponent");
}

void ALetheGameState::BeginPlay()
{
	Super::BeginPlay();
	
	AbilityResolverComponent->OnEnemyAbilityActivatedDelegate.AddUObject(this, &ThisClass::OnEnemyAbilityActivated);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	AbilityResolverComponent->OnEnemyAbilityActivatedDelegate.RemoveAll(this);
	
	Super::EndPlay(EndPlayReason);
}

void ALetheGameState::GoDrawPhase()
{
	SetPhase(EPhaseState::DrawPhase);
}

void ALetheGameState::GoPlayerTurnPhase()
{
	SetPhase(EPhaseState::PlayerTurnPhase);
}

void ALetheGameState::GoEnemyTurnPhase()
{
	SetPhase(EPhaseState::EnemyTurnPhase);
}

void ALetheGameState::SetPhase(const EPhaseState NewPhase)
{
	const EPhaseState OldPhase = CurrentTurnState;
	if (OldPhase == NewPhase)
	{
		return;
	}
	
	CurrentTurnState = NewPhase;

	if (CurrentTurnState == EPhaseState::DrawPhase)
	{
		// MoveConsumed 태그를 제거한 후 AIController의 SelectAbility 로직이 시작될 수 있도록 순서 보장을 위해 분리된 콜백을 호출합니다.
		OnDrawPhaseStartedDelegate.Broadcast();
	}
	
	OnChangeTurnStateDelegate.Broadcast(OldPhase, CurrentTurnState);
	
	if (CurrentTurnState == EPhaseState::EnemyTurnPhase)
	{
		AbilityResolverComponent->SortEnemyAbilityActivationData();
		AbilityResolverComponent->StartActivateEnemyAbility();
	}
}

EPhaseState ALetheGameState::GetTurnPhase() const
{
	return CurrentTurnState;
}

void ALetheGameState::AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddPlayerAbilityActivationData(ActivationData);
}

void ALetheGameState::AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const
{
	AbilityResolverComponent->AddEnemyAbilityActivationData(ActivationData);
}

void ALetheGameState::OnEnemyAbilityActivated(AActor* AbilityInstigator) const
{
	OnActivateEnemyAbilityDelegate.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityEnded(const bool bSuccess) const
{
	AbilityResolverComponent->OnAbilityEnded(bSuccess);
}

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsProgressingPlayerAbility() const
{
	return AbilityResolverComponent->IsActivatingPlayerAbility();
}
