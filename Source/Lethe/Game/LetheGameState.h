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
	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePhaseStateSignature, const EPhaseState /* OldState */, const EPhaseState /* NewState */);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALetheGameState();
	
	void GoDrawPhase();
	void GoPlayerTurnPhase();
	void GoEnemyTurnPhase();

	EPhaseState GetTurnPhase() const;

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const;
	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const;
	void SetTargetTileForEnemy(const int32 Priority, ATile* TargetTile) const;

	void OnAbilityEnded(const bool bSuccess) const;

	UAbilityResolverComponent* GetAbilityResolverComponent() const;
	bool IsProgressingPlayerAbility() const;

private:
	void SetPhase(const EPhaseState NewPhase);

public:
	FOnChangePhaseStateSignature OnChangeTurnStateDelegate;

private:
	EPhaseState CurrentTurnState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;
};
