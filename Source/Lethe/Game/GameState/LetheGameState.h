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
	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData) const;

	void OnEnemyAbilityActivated(AActor* AbilityInstigator) const;

	UFUNCTION(BlueprintCallable)
	void OnAbilityEnded(const bool bSuccess);

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

public:
	FOnChangePhaseStateSignature OnChangePhaseStateDelegate;
	FOnActivateEnemyAbilitySignature OnActivateEnemyAbilityDelegate;

private:
	EPhaseState CurrentPhaseState = EPhaseState::None;

	UPROPERTY()
	TObjectPtr<UAbilityResolverComponent> AbilityResolverComponent;

	// 우선순위대로 정렬되는 현재 스폰된 적들입니다.
	TArray<TWeakObjectPtr<AEnemyCharacterBase>> RegisteredEnemies;
	int32 CurrentEnemyIndex = 0;
	uint8 bIsEnemyPlanning : 1 = false;
};
