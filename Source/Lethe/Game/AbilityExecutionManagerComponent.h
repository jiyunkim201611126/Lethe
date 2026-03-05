// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "AbilityExecutionManagerComponent.generated.h"

class ATile;

UCLASS()
class LETHE_API UAbilityExecutionManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityExecutionManagerComponent();

	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData);
	void SetTargetTile(const int32 Priority, ATile* TargetTile);

	void OnEnemyTurnPhaseStarted();

private:
	void TryUseNextAbility();
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

private:
	// 우선순위를 기록하는 TArray로, EnemyTurn 시작 시 정렬하고 순서대로 참조합니다.
	TArray<int32> Priorities;

	// Key가 우선순위인 TMap으로, 딜레이 참조와 SetTargetTile 시 O(1)참조를 위해 선언되었습니다.
	TMap<int32, FAbilityActivationData> EnemyAbilityActivationData;

	int32 CurrentPriorityIndex;
	TWeakObjectPtr<UAbilitySystemComponent> CurrentActivationASC;
};
