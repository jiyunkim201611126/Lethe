// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "AbilityExecutionManagerComponent.generated.h"

class ATile;

UENUM()
enum class EAbilityExecutionResult : uint8
{
	AllAbilityUsed,
	FailedLogicError,
	FailedFatal,
	Success
};

UCLASS()
class LETHE_API UAbilityExecutionManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityExecutionManagerComponent();

	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData);
	void SetTargetTile(const int32 Priority, ATile* TargetTile);

	void StartUseAbility();

private:
	EAbilityExecutionResult TryUseNextAbility();
	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

private:
	// 우선순위를 기록하는 TArray로, Heap 관련 함수만 사용합니다.
	TArray<int32> EnemyAbilityActivationPriorities;

	// Key가 우선순위인 TMap으로, 딜레이 참조와 SetTargetTile 시 O(1)참조를 위해 선언되었습니다.
	TMap<int32, FAbilityActivationData> EnemyAbilityActivationData;

	TWeakObjectPtr<UAbilitySystemComponent> CurrentActivationASC;
};
