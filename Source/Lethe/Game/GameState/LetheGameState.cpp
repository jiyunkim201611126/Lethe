// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameState.h"

#include "AbilityResolverComponent.h"
#include "CardContainerComponent.h"
#include "Lethe/LetheLog.h"
#include "TurnManagerComponent.h"

ALetheGameState::ALetheGameState()
{
	TurnManagerComponent = CreateDefaultSubobject<UTurnManagerComponent>("TurnManagerComponent");
	AbilityResolverComponent = CreateDefaultSubobject<UAbilityResolverComponent>("AbilityResolverComponent");
	CardContainerComponent = CreateDefaultSubobject<UCardContainerComponent>("CardContainerComponent");
}

void ALetheGameState::BeginPlay()
{
	Super::BeginPlay();

	TurnManagerComponent->OnTurnPhaseStateChanged.AddUObject(this, &ThisClass::OnTurnPhaseStateChanged);
	TurnManagerComponent->Initialize(AbilityResolverComponent);

	AbilityResolverComponent->OnTryActivateEnemyAbility.BindUObject(this, &ThisClass::OnTryActivateEnemyAbility);
	AbilityResolverComponent->OnResolveUseCard.BindUObject(this, &ThisClass::OnResolveUseCard);

	CardContainerComponent->OnHandFull.AddUObject(this, &ThisClass::OnHandFull);
}

void ALetheGameState::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	TurnManagerComponent->OnTurnPhaseStateChanged.RemoveAll(this);
	TurnManagerComponent->Deinitialize();

	AbilityResolverComponent->OnTryActivateEnemyAbility.Unbind();
	AbilityResolverComponent->OnResolveUseCard.Unbind();

	CardContainerComponent->OnHandFull.RemoveAll(this);

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

void ALetheGameState::InitializeCardContainer(const TArray<FGameplayTag>& CharacterOrder, const TMap<FGameplayTag, FSavedCharacterDeck>& EquippedDecks, const int32 ShuffleSeed) const
{
	CardContainerComponent->Initialize(CharacterOrder, EquippedDecks, ShuffleSeed);
}

UCardContainerComponent* ALetheGameState::GetCardContainerComponent() const
{
	return CardContainerComponent;
}

void ALetheGameState::RequestEndPlayerMovePhase() const
{
	TurnManagerComponent->RequestEndPlayerMovePhase();
}

void ALetheGameState::RequestEndPlayerTurn() const
{
	TurnManagerComponent->RequestEndPlayerTurn();
}

void ALetheGameState::OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState) const
{
	// 뷰(CardStage 등)가 갱신된 컨테이너 상태를 보고 반응할 수 있도록, 외부 브로드캐스트보다 먼저 모델을 갱신합니다.
	CardContainerComponent->HandleTurnPhaseStateChanged(OldTurnPhaseState, NewTurnPhaseState, TurnManagerComponent->IsBattlePhase());
	OnChangeTurnPhaseState.Broadcast(OldTurnPhaseState, NewTurnPhaseState);
}

void ALetheGameState::OnHandFull() const
{
	TurnManagerComponent->NotifyDrawPhaseCompleted();
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

#if WITH_EDITOR
void ALetheGameState::DumpTurnDebugInfo() const
{
	TStringBuilder<32768> Builder;
	const UWorld* World = GetWorld();

	Builder.Append(TEXT("LETHE 턴 디버그 스냅샷\n"));
	Builder.Append(TEXT("============================================================\n"));
	Builder.Appendf(TEXT("GameState = %s, World = %s, Frame = %llu, GameTime = %.3f초, RealTime = %.3f초\n"),
		*GetNameSafe(this), *GetNameSafe(World), GFrameCounter,
		World ? World->GetTimeSeconds() : 0.0,
		World ? World->GetRealTimeSeconds() : 0.0);

	if (TurnManagerComponent)
	{
		TurnManagerComponent->AppendDebugSnapshot(Builder);
	}
	else
	{
		Builder.Append(TEXT("\n[TurnManagerComponent]\n  [!] nullptr\n"));
	}

	if (AbilityResolverComponent)
	{
		AbilityResolverComponent->AppendDebugSnapshot(Builder);
	}
	else
	{
		Builder.Append(TEXT("\n[AbilityResolverComponent]\n  [!] nullptr\n"));
	}

	Builder.Append(TEXT("============================================================\n"));
	UE_LOG(LogLetheGameState, Warning, TEXT("%s"), Builder.ToString());
}
#endif
