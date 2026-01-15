// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "LetheGameState.generated.h"

UENUM()
enum class EPlayerPhaseState : uint8
{
	None,
	DrawPhase,
	BattlePhase,
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChangePlayerPhaseStateSignature, const EPlayerPhaseState);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	void GoDrawPhase();
	void GoBattlePhase();

public:
	FOnChangePlayerPhaseStateSignature OnChangePlayerTurnStateDelegate;

private:
	EPlayerPhaseState CurrentPlayerTurnState = EPlayerPhaseState::None;
};
