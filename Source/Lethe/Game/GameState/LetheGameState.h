// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilityResolverComponent.h"
#include "GameFramework/GameStateBase.h"
#include "LetheGameState.generated.h"

class AEnemyCharacterBase;
class ATile;

UENUM()
enum class EPhaseState : uint8
{
	None,
	EnemyPlanningPhase,

	/** 비전투 페이즈 */
	PlayerMovePhase,

	/** 전투 페이즈 */
	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePhaseState, const EPhaseState /* OldState */, const EPhaseState /* NewState */);
DECLARE_DELEGATE_OneParam(FOnPlayerMoveResolved, const AActor* /* MovedCharacter */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyAbilityActivated, AActor* /* Instigator */);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALetheGameState();

	void RegisterEnemy(AEnemyCharacterBase* Enemy);
	void RegisterCombatEnemy(AEnemyCharacterBase* Enemy);
	void UnregisterEnemy(AEnemyCharacterBase* Enemy);

	void GoEnemyPlanningPhase();
	void GoPlayerMovePhase();
	void GoDrawPhase();
	void GoPlayerTurnPhase();
	void GoEnemyTurnPhase();

	EPhaseState GetPhaseState() const;

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData, const bool bStartImmediately = true) const;
	void StartActivatePlayerAbility() const;
	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData);
	void ActivateAbility(FAbilityActivationData& ActivationData) const;

	void OnActivateEnemyAbility(AActor* AbilityInstigator) const;

	/** Ability를 Activate하는 데까진 성공했으나, 모종의 이유(층 수 차이, 이미 사망한 적 등)로 CardAbility에서 반려한 경우 호출되는 함수입니다. */
	void OnAbilityActivationFailed() const;

	/** 플레이어 캐릭터의 이동을 성공적으로 마친 경우 호출하는 함수입니다. */
	UFUNCTION(BlueprintCallable)
	void OnResolvePlayerMove(const AActor* MovedCharacter) const;

	/**
	 * Enemy Plan 단계에서 MoveAbility가 끝났거나, MoveAbility를 사용할 필요가 없을 때 호출합니다.
	 * GA_Move에선 EndAbility 직전에 호출하는 함수로, 현재는 '적은 한 번에 여러 Ability를 사용하지 않는다.'는 전제하에 정상 작동하는 상태입니다.
	 * 만약 적이 MoveAbility를 연속으로 발동한다면 문제가 생길 수 있으나, 프로젝트 정책상 그럴 일이 없어 현재 해결해두지 않았습니다.
	 */
	UFUNCTION(BlueprintCallable)
	void OnResolveEnemyPlanMove();

	void OnPlanTimerEnded();

	UAbilityResolverComponent* GetAbilityResolverComponent() const;
	bool IsProgressingPlayerAbility() const;

protected:
	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~ End of AActor Interface

private:
	void SetPhase(const EPhaseState NewPhase);

	void ProcessCurrentEnemyPlan();
	void OnFinishActivationQueue();

	bool ShouldGoCombatPhase() const;

public:
	FOnChangePhaseState OnChangePhaseState;
	FOnPlayerMoveResolved OnPlayerMoveResolved;
	FOnEnemyAbilityActivated OnEnemyAbilityActivated;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> DummyActorClass;

	UPROPERTY(EditDefaultsOnly)
	float EnemyAbilityDelayTime = 0.5f;

private:
	EPhaseState CurrentPhaseState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	/** 우선순위대로 정렬되는 현재 스폰된 적들입니다. */
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> SpawnedEnemies;
	int32 CurrentEnemyAbilityProcessIndex = 0;
	
	/**
	 * 등록 후 거의 즉시 실행되는 PlayerAbility와는 달리, EnemyAbility는 예고 후 플레이어의 조작에 의해 취소되거나 조정될 수 있습니다.
	 * 따라서 ResolverComponent로 즉시 넘기지 않고, 아래 배열에 들고 있다가 플레이어의 조작이 끝나면 한 번에 넘겨 사용합니다.
	 */
	TArray<FAbilityActivationData> ReservedEnemyAbilityActivationData;
	FTimerHandle PlanTimerHandle;

	/** 현재 전투에 참여 중인 적을 기록하는 TSet으로, Phase 판별에 사용합니다. */
	TSet<TWeakObjectPtr<AEnemyCharacterBase>> CurrentCombatEnemies;
};
