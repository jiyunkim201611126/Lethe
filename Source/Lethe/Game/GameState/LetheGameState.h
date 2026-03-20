// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilityResolverComponent.h"
#include "GameFramework/GameStateBase.h"
#include "LetheGameState.generated.h"

class ATile;

/**
 * 페이즈가 1바퀴 도는 것을 Round라고 지칭하며, EnemyMovePhase가 Round의 시작 첫 Phase입니다.
 * PlayerMovePhase는 EnemyMovePhase가 끝난 뒤 비전투 상황일 때 돌입합니다.
 * 만약 EnemyMovePhase가 끝난 후 전투 상황이라면 PlayerMovePhase를 스킵하고 DrawPhase로 돌입합니다.
 * 이 경우 플레이어는 PlayerTurnPhase에 캐릭터들을 움직일 수 있습니다.
 */
UENUM()
enum class EPhaseState : uint8
{
	None,
	EnemyMovePhase,
	PlayerMovePhase,

	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};

DECLARE_MULTICAST_DELEGATE(FOnRoundStartedSignature);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePhaseStateSignature, const EPhaseState /* OldState */, const EPhaseState /* NewState */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActivateEnemyAbilitySignature, AActor* /* Instigator */);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALetheGameState();

	void GoEnemyMovePhase();
	void GoPlayerPhase();
	void GoPlayerTurnPhase();

	void RequestTurnEnd();

	void RegisterEnemy(AActor* Enemy);
	void RemovePendingEnemyMove(AActor* Enemy);
	void OnAllEnemyAbilityResolved();

	EPhaseState GetPhaseState() const;

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

	void RebuildCurrentInBattleEnemies();

public:
	FOnRoundStartedSignature OnRoundStartedDelegate;
	FOnChangePhaseStateSignature OnChangePhaseStateDelegate;
	FOnActivateEnemyAbilitySignature OnActivateEnemyAbilityDelegate;

private:
	EPhaseState CurrentPhaseState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	// 현재 맵에 스폰된 모든 적들입니다.
	TSet<TWeakObjectPtr<AActor>> RegisteredEnemies;
	// MoveAbility 예약을 아직 걸지 않은 적들입니다.
	TSet<TWeakObjectPtr<AActor>> PendingMoveEnemies;
	// 현재 전투에 참여 중인 적들입니다.
	TSet<TWeakObjectPtr<AActor>> CurrentInBattleEnemies;
};
