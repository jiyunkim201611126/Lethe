// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "AbilityResolverComponent.generated.h"

class ATile;

UENUM()
enum class ETryAbilityActivationResult : uint8
{
	AllAbilityUsed,
	FailedLogicError,
	FailedFatal,
	Success
};

DECLARE_DELEGATE_TwoParams(FOnUseCardResolved, const int32 /* HandIndex */, const bool /* bSuccess */);

UCLASS()
class LETHE_API UAbilityResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityResolverComponent();

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData);

	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData);
	void SetTargetTile(const int32 Priority, ATile* TargetTile);

	void StartUsePlayerAbility();
	void OnPlayerAbilityUsed(const ETryAbilityActivationResult Result);
	void ProcessAllPlayerAbilitiesFailed();
	
	void StartUseEnemyAbility();
	void OnEnemyAbilityUsed(const ETryAbilityActivationResult Result);
	void ResetEnemyData();
	
	void OnAbilityEnded(const bool bSuccess);

	bool IsProgressingPlayerAbility() const;

private:
	ETryAbilityActivationResult TryUseNextPlayerAbility();
	ETryAbilityActivationResult TryUseNextEnemyAbility();
	ETryAbilityActivationResult TryUseAbility(FAbilityActivationData* ActivationData) const;

public:
	FOnUseCardResolved OnUseCardResolved;

private:
	// Array지만 사실상 Queue의 작동 방식을 갖습니다.
	// 최대 8개의 원소를 갖기 때문에 0번째 인덱스를 제거하는 비용이 그리 크지 않으며, TQueue는 Dequeue할 때마다 값복사가 발생하기 때문에 TArray로 구현합니다. 
	TArray<FAbilityActivationData> PlayerAbilityActivationData;
	uint8 bIsProgressingPlayerAbility : 1 = false;
	
	// Enemy AI의 Ability 사용 우선순위를 기록하는 TArray로, Heap 관련 함수만 사용합니다.
	TArray<int32> EnemyAbilityActivationPriorities;

	// Key가 우선순위인 TMap으로, 딜레이 참조 및 SetTargetTile 시 O(1)참조를 위해 선언되었습니다.
	TMap<int32, FAbilityActivationData> EnemyAbilityActivationData;

	ETeamSide CurrentActivationCharacterTeamSide = ETeamSide::None;
};
