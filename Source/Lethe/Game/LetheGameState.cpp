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
	
	OnChangeTurnStateDelegate.Broadcast(OldPhase, NewPhase);
	
	if (NewPhase == EPhaseState::EnemyTurnPhase)
	{
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

void ALetheGameState::SetTargetTileForEnemy(const int32 Priority, ATile* TargetTile) const
{
	AbilityResolverComponent->SetTargetTileForEnemy(Priority, TargetTile);
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
