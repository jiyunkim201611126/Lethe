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

	DrawPhase,
	PlayerTurnPhase,
	EnemyTurnPhase,
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnChangePhaseStateSignature, const EPhaseState /* OldState */, const EPhaseState /* NewState */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActivateEnemyAbilitySignature, AActor* /* Instigator */);

UCLASS()
class LETHE_API ALetheGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ALetheGameState();

	void RegisterEnemy(AEnemyCharacterBase* Enemy);

	void GoEnemyPlanningPhase();
	void GoDrawPhase();
	void GoPlayerTurnPhase();
	void GoEnemyTurnPhase();

	EPhaseState GetPhaseState() const;

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData) const;
	
	void ActivateEnemyAbility(FAbilityActivationData& ActivationData) const;
	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData);

	void OnEnemyAbilityActivated(AActor* AbilityInstigator) const;

	// 일반적인 Ability 종료를 Resolver에 전달할 때 사용합니다.
	UFUNCTION(BlueprintCallable)
	void OnAbilityEnded(const bool bSuccess);

	// Enemy Plan 단계에서 MoveAbility가 끝났거나, MoveAbility를 사용할 필요가 없을 때 호출합니다.
	UFUNCTION(BlueprintCallable)
	void OnEnemyPlanMoveResolved();

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
	void OnEnemyExecutionQueueFinished();

public:
	FOnChangePhaseStateSignature OnChangePhaseStateDelegate;
	FOnActivateEnemyAbilitySignature OnActivateEnemyAbilityDelegate;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> DummyActorClass;

	UPROPERTY(EditDefaultsOnly)
	float EnemyAbilityDelayTime = 0.5f;

private:
	EPhaseState CurrentPhaseState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	// 우선순위대로 정렬되는 현재 스폰된 적들입니다.
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> RegisteredEnemies;
	int32 CurrentEnemyIndex = 0;

	/**
	 * 등록 후 거의 즉시 실행되는 PlayerAbility와는 달리, EnemyAbility는 예고 후 플레이어의 조작에 의해 취소되거나 조정될 수 있습니다.
	 * 따라서 ResolverComponent로 즉시 넘기지 않고, 아래 배열에 들고 있다가 플레이어의 조작이 끝나면 한 번에 넘겨 사용합니다.
	 */
	TArray<FAbilityActivationData> ReservedEnemyAbilityActivationData;

	FTimerHandle PlanTimerHandle;
};
