// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

#include "AbilityResolverComponent.h"

ALetheGameState::ALetheGameState()
{
	AbilityResolverComponent = CreateDefaultSubobject<UAbilityResolverComponent>("AbilityResolverComponent");
	TurnManagerComponent = CreateDefaultSubobject<UTurnManagerComponent>("TurnManagerComponent");
}

void ALetheGameState::BeginPlay()
{
	Super::BeginPlay();

	TurnManagerComponent->OnChangePhaseState.AddUObject(this, &ThisClass::OnTurnPhaseChanged);
	TurnManagerComponent->Initialize(AbilityResolverComponent);

	AbilityResolverComponent->OnAttemptEnemyAbility.BindUObject(this, &ThisClass::OnAttemptEnemyAbility);
	AbilityResolverComponent->OnResolveUseCard.BindUObject(this, &ThisClass::OnResolveUseCard);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TurnManagerComponent->OnChangePhaseState.RemoveAll(this);
	TurnManagerComponent->Deinitialize();

	AbilityResolverComponent->OnAttemptEnemyAbility.Unbind();
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

void ALetheGameState::OnAttemptEnemyAbility(AActor* AbilityInstigator) const
{
	OnEnemyAbilityAttempt.Broadcast(AbilityInstigator);
}

void ALetheGameState::OnAbilityActivationFailed() const
{
	AbilityResolverComponent->OnAbilityActivationFailed();
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

UAbilityResolverComponent* ALetheGameState::GetAbilityResolverComponent() const
{
	return AbilityResolverComponent;
}

bool ALetheGameState::IsResolvingPlayerAbility() const
{
	return AbilityResolverComponent->IsResolvingPlayerAbility();
}

TArray<AActor*> ALetheGameState::GetPlayerCharacters() const
{
	return TurnManagerComponent->GetPlayerCharacters();
}

bool ALetheGameState::IsBattlePhase() const
{
	return TurnManagerComponent->IsBattlePhase();
}

void ALetheGameState::OnTurnPhaseChanged(const EPhaseState OldPhaseState, const EPhaseState NewPhaseState) const
{
	OnChangePhaseState.Broadcast(OldPhaseState, NewPhaseState);
}

void ALetheGameState::OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess)
{
	OnCardUseResolved.ExecuteIfBound(HandSlotIndex, bSuccess);
	TurnManagerComponent->NotifyCardUseResolved();
}
