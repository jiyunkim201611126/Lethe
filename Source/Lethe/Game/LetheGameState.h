// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LetheGameState.generated.h"

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
	void GoDrawPhase();
	void GoPlayerTurnPhase();
	void GoEnemyTurnPhase();

	EPhaseState GetTurnPhase() const;

private:
	void SetPhase(const EPhaseState NewPhase);

public:
	FOnChangePhaseStateSignature OnChangeTurnStateDelegate;

private:
	EPhaseState CurrentTurnState = EPhaseState::None;
};
