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
	Success,
	
	AllAbilityUsed,
	
	// 잘못된 로직 작성으로 인한 실패의 경우 반환받게 됩니다.
	FailedLogicError,
	
	// 강제 종료, 엔진상 버그 등으로 인한 실패의 경우 반환받게 됩니다.
	FailedFatal,
	
	// 코스트 부족 혹은 모종의 이유로 인해 Ability가 GAS상 문제로 작동에 실패한 경우 반환받게 됩니다.
	FailedNotActivated,
	
	// Enemy의 MoveAbility 사용 시 TargetTile이 없는 경우 반환받게 됩니다.
	FailedNoneTargetTileToMove,
};

DECLARE_DELEGATE_TwoParams(FOnUseCardResolved, const int32 /* HandIndex */, const bool /* bSuccess */);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnEnemyAbilityActivated, AActor* /* Instigator */);

UCLASS()
class LETHE_API UAbilityResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityResolverComponent();

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData);
	void StartActivatePlayerAbility();
	void HandlePlayerAbilityActivationResult(const ETryAbilityActivationResult Result);
	void ProcessAllPlayerAbilitiesFailed();

	void AddEnemyAbilityActivationData(const FAbilityActivationData& ActivationData);
	void SetTargetTileForEnemy(const int32 Priority, ATile* TargetTile);
	void StartActivateEnemyAbility();
	void HandleEnemyAbilityActivationResult(const ETryAbilityActivationResult Result);
	void ResetEnemyActivationData();
	
	void OnAbilityEnded(const bool bSuccess);

	bool IsActivatingPlayerAbility() const;

private:
	ETryAbilityActivationResult TryActivateNextPlayerAbility();
	ETryAbilityActivationResult TryActivateNextEnemyAbility();
	ETryAbilityActivationResult TryActivateAbility(FAbilityActivationData* ActivationData) const;

public:
	FOnUseCardResolved OnUseCardResolved;
	FOnEnemyAbilityActivated OnEnemyAbilityActivatedDelegate;

private:
	// Array지만 사실상 Queue의 작동 방식을 갖습니다.
	// 최대 8개의 원소를 갖기 때문에 0번째 인덱스를 제거하는 비용이 그리 크지 않으며, TQueue는 Dequeue할 때마다 값복사가 발생하기 때문에 TArray로 구현합니다. 
	TArray<FAbilityActivationData> PlayerAbilityActivationData;
	uint8 bIsActivatingPlayerAbility : 1 = false;
	
	// Enemy AI의 Ability 사용 우선순위를 기록하는 TArray로, Heap 관련 함수만 사용해 우선순위 큐로 구현합니다.
	TArray<int32> EnemyAbilityActivationPriorities;

	// Key가 우선순위인 TMap으로, 딜레이 참조 및 SetTargetTile 시 O(1)참조를 위해 선언되었습니다.
	TMap<int32, FAbilityActivationData> EnemyAbilityActivationData;

	ETeamSide CurrentActivationCharacterTeamSide = ETeamSide::None;
};
