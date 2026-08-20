// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "TurnManagerComponent.h"
#include "LetheGameState.generated.h"

class AEnemyCharacterBase;
class UAbilityResolverComponent;

DECLARE_DELEGATE_OneParam(FOnPlayerMoveResolved, AActor* /* MovedCharacter */);
DECLARE_DELEGATE_TwoParams(FOnCardUseResolved, const int32 /* HandSlotIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyAbilityAttempt, AActor* /* Instigator */);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALetheGameState();

	void RegisterPlayerCharacter(AActor* PlayerCharacter) const;
	void RegisterEnemy(AEnemyCharacterBase* Enemy) const;
	void RegisterCombatEnemy(AEnemyCharacterBase* Enemy) const;
	void UnregisterEnemy(AEnemyCharacterBase* Enemy) const;

	/** 게임 시작 후 타일, 캐릭터 스폰, Ability 부여 등의 모든 사전 작업이 완료되면 호출해 본격적으로 턴제 흐름을 시작합니다. */
	void StartTurnFlow() const;
	void RequestEndPlayerMovePhase() const;
	void NotifyDrawPhaseCompleted() const;
	void RequestEndPlayerTurn() const;

	void EnqueuePlayerAbilityActivationContext(FAbilityActivationContext&& ActivationContext, const bool bStartImmediately = true) const;
	/** PlayerMovePhase에만 사용하는 함수로, 모든 MoveAbility ActivationContext를 밀어넣은 후 호출합니다. */
	void StartActivatePlayerMoveAbilities() const;
	void EnqueueEnemyAbilityActivationContext(const FAbilityActivationContext& ActivationContext) const;
	void ActivateAbility(FAbilityActivationContext& ActivationContext, const ETeamSide TeamSide) const;

	void OnAttemptEnemyAbility(AActor* AbilityInstigator) const;

	/** Ability를 Activate하는 데까진 성공했으나, 모종의 이유(층 수 차이, 이미 사망한 적 등)로 CardAbility에서 반려한 경우 호출되는 함수입니다. */
	void OnAbilityActivationFailed() const;

	/** 플레이어 캐릭터의 이동을 성공적으로 마친 경우 호출하는 함수입니다. */
	UFUNCTION(BlueprintCallable)
	void NotifyPlayerMoveResolved(AActor* MovedCharacter) const;

	/**
	 * 현재는 '적은 한 번에 여러 Ability를 사용하지 않는다.'는 전제하에 정상 작동하는 상태입니다.
	 * 만약 적이 MoveAbility를 연속으로 발동한다면 문제가 생길 수 있으나, 프로젝트 정책상 그럴 일이 없어 현재 해결해두지 않았습니다.
	 * 그 외에 경로 생성 실패나 AIController의 부재 등의 상황에서도 호출하는데, 일반적으로 발생하지 않는 상황입니다.
	 */
	UFUNCTION(BlueprintCallable)
	void NotifyEnemyPlanResolved() const;

	void NotifyPlayerMovePlanChanged() const;

	UAbilityResolverComponent* GetAbilityResolverComponent() const;
	bool IsResolvingPlayerAbility() const;

	UFUNCTION(BlueprintCallable)
	TArray<AActor*> GetPlayerCharacters() const;

	bool IsBattlePhase() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

private:
	void OnTurnPhaseChanged(const EPhaseState OldPhaseState, const EPhaseState NewPhaseState) const;
	void OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess);

public:
	FOnChangePhaseState OnChangePhaseState;
	FOnPlayerMoveResolved OnPlayerMoveResolved;
	FOnCardUseResolved OnCardUseResolved;
	FOnEnemyAbilityAttempt OnEnemyAbilityAttempt;

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	UPROPERTY()
	TObjectPtr<UTurnManagerComponent> TurnManagerComponent;
};
