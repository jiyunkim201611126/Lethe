// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "Lethe/Data/TurnPhaseState.h"
#include "TurnManagerComponent.generated.h"

class AEnemyCharacterBase;
class ICombatInterface;
class UAbilityResolverComponent;

UCLASS()
class LETHE_API UTurnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTurnManagerComponent();

	void Initialize(UAbilityResolverComponent* InAbilityResolverComponent);
	void Deinitialize();

	void RegisterPlayerCharacter(AActor* PlayerCharacter);
	void RegisterEnemy(AEnemyCharacterBase* Enemy);
	void RegisterCombatEnemy(AEnemyCharacterBase* Enemy);
	void UnregisterEnemy(AEnemyCharacterBase* Enemy);

	void StartTurnFlow();
	void RequestEndPlayerMovePhase();
	void NotifyDrawPhaseCompleted();
	void RequestEndPlayerTurn();

	void NotifyAbilityQueueCompleted();
	void NotifyCardUseResolved();
	void NotifyEnemyPlanResolved();
	void NotifyPlayerMovePlanChanged();

	void EnqueueEnemyAbilityActivationContext(const FAbilityActivationContext& ActivationContext);

	bool IsBattlePhase() const;

	TArray<AActor*> GetPlayerCharacters() const;

private:
	bool TryTransitionToTurnPhaseState(const ETurnPhaseState NewTurnPhaseState);
	bool CanTransitionToTurnPhaseState(const ETurnPhaseState NewTurnPhaseState) const;
	void EnterTurnPhaseState(const ETurnPhaseState TurnPhaseState);

	void StartEnemyPlanPhase();
	void StartEnemyTurnPhase();
	void ProcessCurrentEnemyPlan();
	void OnPlanTimerEnded();

	bool HasAnyCombatEnemy() const;

public:
	FOnChangeTurnPhaseState OnTurnPhaseStateChanged;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Turn")
	float EnemyAbilityDelayTime = 0.5f;

private:
	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	UPROPERTY(VisibleInstanceOnly, Category = "Turn")
	ETurnPhaseState CurrentTurnPhaseState = ETurnPhaseState::None;

	TArray<TScriptInterface<ICombatInterface>> PlayerCharacters;

	/** 해당 변수가 true인 경우 PlayerMovePhase 종료 요청 시 한 번 보류합니다. */
	uint8 bShouldDeferEndPlayerMovePhase : 1 = true;

	/** 우선순위대로 정렬되는 현재 스폰된 적들입니다. */
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> SpawnedEnemies;
	int32 CurrentEnemyAbilityProcessIndex = 0;

	/**
	 * 등록 후 거의 즉시 실행되는 PlayerAbility와는 달리, EnemyAbility는 예고 후 플레이어의 조작에 의해 취소되거나 조정될 수 있습니다.
	 * 따라서 ResolverComponent로 즉시 넘기지 않고, 아래 배열에 들고 있다가 플레이어의 조작이 끝나면 한 번에 넘겨 사용합니다.
	 */
	TArray<FAbilityActivationContext> ReservedEnemyAbilityActivationContexts;
	FTimerHandle PlanTimerHandle;

	/** 현재 전투에 참여 중인 적을 기록하는 TSet으로, TurnPhaseState 판별에 사용합니다. */
	TSet<TWeakObjectPtr<AEnemyCharacterBase>> CurrentCombatEnemies;
};
