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
DECLARE_MULTICAST_DELEGATE_OneParam(FOnActivateEnemyAbility, AActor* /* Instigator */);
DECLARE_DELEGATE(FOnFinishEnemyActivationQueue);

UCLASS()
class LETHE_API UAbilityResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityResolverComponent();

	void SetDummyActor(AActor* InDummyActor);

	void AddPlayerAbilityActivationData(const FAbilityActivationData& ActivationData);
	void StartActivatePlayerAbility();
	void HandlePlayerAbilityActivationResult(const ETryAbilityActivationResult Result);
	void ProcessAllPlayerAbilitiesFailed();

	// Queue와 관계없이 Ability를 즉시 발동할 때 사용하는 함수로, MoveAbility 등을 사용할 때 호출합니다.
	void ActivateEnemyAbility(FAbilityActivationData& ActivationData);
	
	void SetEnemyAbilityActivationData(TArray<FAbilityActivationData>&& ActivationData);
	void SortEnemyAbilityActivationData();
	void StartActivateEnemyAbility();
	void HandleEnemyAbilityActivationResult(const ETryAbilityActivationResult Result);
	void ResetEnemyActivationData();
	
	void OnAbilityActivationFailed();

	bool IsActivatingPlayerAbility() const;

private:
	ETryAbilityActivationResult TryActivateNextPlayerAbility();
	ETryAbilityActivationResult TryActivateNextEnemyAbility();
	ETryAbilityActivationResult TryActivateAbility(FAbilityActivationData* ActivationData);

	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

public:
	FOnUseCardResolved OnCardUseResolved;
	FOnActivateEnemyAbility OnActivateEnemyAbility;
	FOnFinishEnemyActivationQueue OnFinishEnemyActivationQueue;

private:
	UPROPERTY()
	TObjectPtr<AActor> DummyActor;
	
	// Array지만 사실상 Queue의 작동 방식을 갖습니다.
	// TQueue는 Dequeue할 때마다 값복사가 발생하기 때문에 TArray로 구현합니다.
	TArray<FAbilityActivationData> PlayerAbilityActivationData;
	// PlayerAbilityQueue가 작동 중인지를 표현하는 변수입니다.
	uint8 bIsResolvingPlayerAbility : 1 = false;

	TArray<FAbilityActivationData> EnemyAbilityActivationData;

	ETeamSide CurrentActivationCharacterTeamSide = ETeamSide::None;
	
	TWeakObjectPtr<UAbilitySystemComponent> CurrentActivationASC;

	FDelegateHandle OnAbilityEndedDelegate;
};
