// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

#include "AbilityResolverComponent.h"
#include "TurnManagerComponent.h"

ALetheGameState::ALetheGameState()
{
	TurnManagerComponent = CreateDefaultSubobject<UTurnManagerComponent>("TurnManagerComponent");
	AbilityResolverComponent = CreateDefaultSubobject<UAbilityResolverComponent>("AbilityResolverComponent");
}

void ALetheGameState::BeginPlay()
{
	Super::BeginPlay();

	TurnManagerComponent->OnTurnPhaseStateChanged.AddUObject(this, &ThisClass::OnTurnPhaseStateChanged);
	TurnManagerComponent->Initialize(AbilityResolverComponent);

	AbilityResolverComponent->OnTryActivateEnemyAbility.BindUObject(this, &ThisClass::OnTryActivateEnemyAbility);
	AbilityResolverComponent->OnResolveUseCard.BindUObject(this, &ThisClass::OnResolveUseCard);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TurnManagerComponent->OnTurnPhaseStateChanged.RemoveAll(this);
	TurnManagerComponent->Deinitialize();

	AbilityResolverComponent->OnTryActivateEnemyAbility.Unbind();
	AbilityResolverComponent->OnResolveUseCard.Unbind();

	Super::EndPlay(EndPlayReason);
}

void ALetheGameState::RegisterPlayerCharacter(AActor* PlayerCharacter) const
{
	TurnManagerComponent->RegisterPlayerCharacter(PlayerCharacter);
}

void ALetheGameState::RegisterEnemy(AEnemyCharacterBase* Enemy) const
{
	TurnManagerComponent->RegisterEnemy(Enemy);
}

void ALetheGameState::RegisterCombatEnemy(AEnemyCharacterBase* Enemy) const
{
	TurnManagerComponent->RegisterCombatEnemy(Enemy);
}

void ALetheGameState::UnregisterEnemy(AEnemyCharacterBase* Enemy) const
{
	TurnManagerComponent->UnregisterEnemy(Enemy);
}

void ALetheGameState::StartTurnFlow() const
{
	TurnManagerComponent->StartTurnFlow();
}

void ALetheGameState::RequestEndPlayerMovePhase() const
{
	TurnManagerComponent->RequestEndPlayerMovePhase();
}

void ALetheGameState::NotifyDrawPhaseCompleted() const
{
	TurnManagerComponent->NotifyDrawPhaseCompleted();
}

void ALetheGameState::RequestEndPlayerTurn() const
{
	TurnManagerComponent->RequestEndPlayerTurn();
}

void ALetheGameState::OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState) const
{
	OnChangeTurnPhaseState.Broadcast(OldTurnPhaseState, NewTurnPhaseState);
}

void ALetheGameState::EnqueuePlayerAbilityActivationContext(FAbilityActivationContext&& ActivationContext, const bool bStartImmediately) const
{
	AbilityResolverComponent->EnqueuePlayerAbilityActivationContext(MoveTemp(ActivationContext), bStartImmediately);
}

void ALetheGameState::StartActivatePlayerMoveAbilities() const
{
	AbilityResolverComponent->StartActivatePlayerAbility();
}

void ALetheGameState::EnqueueEnemyAbilityActivationContext(const FAbilityActivationContext& ActivationContext) const
{
	TurnManagerComponent->EnqueueEnemyAbilityActivationContext(ActivationContext);
}

void ALetheGameState::ActivateAbility(FAbilityActivationContext& ActivationContext, const ETeamSide TeamSide) const
{
	AbilityResolverComponent->ActivateAbility(ActivationContext, TeamSide);
}

void ALetheGameState::OnTryActivateEnemyAbility(AActor* AbilityInstigator) const
{
	OnEnemyAbilityTriedActivate.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityActivationFailed() const
{
	AbilityResolverComponent->OnAbilityActivationFailed();
}

void ALetheGameState::OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess) const
{
	OnCardUseResolved.ExecuteIfBound(HandSlotIndex, bSuccess);
	TurnManagerComponent->NotifyCardUseResolved();
}

void ALetheGameState::NotifyPlayerMoveResolved(AActor* MovedCharacter) const
{
	OnPlayerMoveResolved.ExecuteIfBound(MovedCharacter);
}

void ALetheGameState::NotifyEnemyPlanResolved() const
{
	TurnManagerComponent->NotifyEnemyPlanResolved();
}

void ALetheGameState::NotifyPlayerMovePlanChanged() const
{
	TurnManagerComponent->NotifyPlayerMovePlanChanged();
}

TArray<AActor*> ALetheGameState::GetPlayerCharacters() const
{
	return TurnManagerComponent->GetPlayerCharacters();
}

bool ALetheGameState::IsResolvingPlayerAbility() const
{
	return AbilityResolverComponent->IsResolvingPlayerAbility();
}

bool ALetheGameState::IsBattlePhase() const
{
	return TurnManagerComponent->IsBattlePhase();
}
