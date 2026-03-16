// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilityResolverComponent.h"
#include "GameFramework/GameStateBase.h"
#include "LetheGameState.generated.h"

class ATile;

UENUM()
enum class EPhaseState : uint8
{
	None,
	EnemyMovePhase,
	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};

DECLARE_MULTICAST_DELEGATE(FOnEnemyMovePhaseStartedSignature);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePhaseStateSignature, const EPhaseState /* OldState */, const EPhaseState /* NewState */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActivateEnemyAbilitySignature, AActor* /* Instigator */);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALetheGameState();

	void GoEnemyMovePhase();
	void GoDrawPhase();
	void GoPlayerTurnPhase();
	void GoEnemyTurnPhase();

	void RegisterEnemy(AActor* InEnemyAI);
	void RemovePendingEnemyMove(AActor* InEnemyAI);
	void OnAllEnemyAbilityResolved();

	EPhaseState GetTurnPhase() const;

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const;
	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const;

	void OnEnemyAbilityActivated(AActor* AbilityInstigator) const;

	UFUNCTION(BlueprintCallable)
	void OnAbilityEnded(const bool bSuccess) const;

	UAbilityResolverComponent* GetAbilityResolverComponent() const;
	bool IsProgressingPlayerAbility() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

private:
	void SetPhase(const EPhaseState NewPhase);

public:
	FOnEnemyMovePhaseStartedSignature OnEnemyMovePhaseStartedDelegate;
	FOnChangePhaseStateSignature OnChangeTurnStateDelegate;
	FOnActivateEnemyAbilitySignature OnActivateEnemyAbilityDelegate;

private:
	EPhaseState CurrentTurnState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	TSet<TWeakObjectPtr<AActor>> RegisteredEnemies;
	TSet<TWeakObjectPtr<AActor>> PendingMoveEnemies;
};
