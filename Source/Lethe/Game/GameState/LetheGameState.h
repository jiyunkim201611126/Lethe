// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Lethe/Data/TurnPhaseState.h"
#include "LetheGameState.generated.h"

enum class ETeamSide : uint8;
class AEnemyCharacterBase;
class UAbilityResolverComponent;
class UTurnManagerComponent;
struct FAbilityActivationContext;

DECLARE_DELEGATE_OneParam(FOnPlayerMoveResolved, AActor* /* MovedCharacter */);
DECLARE_DELEGATE_TwoParams(FOnCardUseResolved, const int32 /* HandSlotIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyAbilityTriedActivate, AActor* /* Instigator */);

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

	/**
	 * 아래 함수들은 특정 페이즈로의 강제 전환이 아니라, 턴 진행 과정에서 발생한 요청이나 완료 사실을 TurnManager에 전달합니다.
	 * 실제 전환은 전환 가능 여부를 턴 상태 기준으로 TurnManager 내부에서 결정해 진행합니다.
	 */
	//~ Begin Turn Control
	/** 플레이어의 이동 페이즈를 종료를 요청합니다. */
	void RequestEndPlayerMovePhase() const;
	/** 카드 드로우 처리가 완료되었음을 알립니다. */
	void NotifyDrawPhaseCompleted() const;
	/** 플레이어 턴의 종료를 요청합니다. */
	void RequestEndPlayerTurn() const;
	//~ End of Turn Control

	/** 플레이어의 Ability 발동을 위한 Context를 밀어넣으며, 인자에 따라 바로 발동하거나 큐에 넣어둡니다. */
	void EnqueuePlayerAbilityActivationContext(FAbilityActivationContext&& ActivationContext, const bool bStartImmediately = true) const;
	/** PlayerMovePhase에만 사용하는 함수로, 모든 MoveAbility ActivationContext를 밀어넣은 후 호출합니다. */
	void StartActivatePlayerMoveAbilities() const;
	/** 적의 Ability 발동을 위한 Context를 밀어넣습니다. */
	void EnqueueEnemyAbilityActivationContext(const FAbilityActivationContext& ActivationContext) const;
	/**
	 * Queue와 관계없이 Ability를 즉시 발동할 때 사용합니다.
	 * 여러 번의 BFS를 수행해야 하는 EnemyPlanPhase 특성상, 모든 AI가 한 번에 예약을 걸면 프레임 드랍이 발생할 확률이 높습니다.
	 * 때문에 EnemyPlanPhase는 AI마다 시간차를 두어 로직이 수행되고, 이는 예약하는 방식으로는 구현이 까다로워 즉시 발동 API를 하나 추가했습니다.
	 */
	void ActivateAbility(FAbilityActivationContext& ActivationContext, const ETeamSide TeamSide) const;

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

	UFUNCTION(BlueprintCallable)
	TArray<AActor*> GetPlayerCharacters() const;

	bool IsResolvingPlayerAbility() const;
	bool IsBattlePhase() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

private:
	void OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState) const;
	void OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess) const;
	void OnTryActivateEnemyAbility(AActor* AbilityInstigator) const;

public:
	FOnChangeTurnPhaseState OnChangeTurnPhaseState;
	FOnPlayerMoveResolved OnPlayerMoveResolved;
	FOnCardUseResolved OnCardUseResolved;
	FOnEnemyAbilityTriedActivate OnEnemyAbilityTriedActivate;

private:
	UPROPERTY()
	TObjectPtr<UTurnManagerComponent> TurnManagerComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

public:
	UFUNCTION(BlueprintCallable, Category = "Debug | Turn", meta = (DevelopmentOnly))
	void DumpTurnDebugInfo() const;
};
