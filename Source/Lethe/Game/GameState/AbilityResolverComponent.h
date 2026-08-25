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

	/** 카드 연속 사용으로 대상을 지정했으나, 모종의 이유(대상의 사망이나 넉백 등으로 해당 위치를 벗어남)로 대상이 없어 발동에 실패한 경우입니다. */
	EmptyTile,
	
	/** 잘못된 로직 작성으로 인한 실패의 경우 반환받게 됩니다. */
	FailedLogicError,
	
	/** 강제 종료, 엔진상 버그 등으로 인한 실패의 경우 반환받게 됩니다. */
	FailedFatal,
	
	/** 코스트 부족 혹은 모종의 이유로 인해 Ability가 GAS상 문제로 작동에 실패한 경우 반환받게 됩니다. */
	FailedNotActivated,
	
	/** Enemy의 MoveAbility 사용 시 TargetTile이 없는 경우 반환받게 됩니다. */
	FailedNoMoveDestination,
};

DECLARE_DELEGATE_TwoParams(FOnResolveUseCard, const int32 /* HandSlotIndex */, const bool /* bSuccess */);
DECLARE_DELEGATE_OneParam(FOnTryActivateEnemyAbility, AActor* /* Instigator */);
DECLARE_DELEGATE(FOnFinishActivationQueue);

UCLASS()
class LETHE_API UAbilityResolverComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityResolverComponent();
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void EnqueuePlayerAbilityActivationContext(FAbilityActivationContext&& ActivationContext, const bool bStartImmediately);
	void StartActivatePlayerAbility();
	void HandlePlayerAbilityActivationResult(const ETryAbilityActivationResult Result);
	void ProcessAllPlayerAbilitiesFailed();
	
	void SetEnemyAbilityActivationContext(TArray<FAbilityActivationContext>&& ActivationContext);
	void StartActivateEnemyAbility();
	void HandleEnemyAbilityActivationResult(const ETryAbilityActivationResult Result);
	void ResetEnemyActivationContext();

	void ActivateAbility(FAbilityActivationContext& ActivationContext, const ETeamSide TeamSide);
	
	void OnAbilityActivationFailed();

	bool IsResolvingPlayerAbility() const;

private:
	ETryAbilityActivationResult TryActivateNextPlayerAbility();
	ETryAbilityActivationResult TryActivateNextEnemyAbility();
	ETryAbilityActivationResult TryActivateAbility(FAbilityActivationContext* ActivationContext);

	bool IsMovementAbility(const FGameplayTag& AbilityTag) const;

	/** Activation 처리 중 보류했던 Ability 종료/실패 콜백을 처리하는 함수입니다. */
	bool ProcessPendingAbilityCallbacks();
	
	void ProcessAbilitySucceeded();
	void ProcessAbilityFailed();

	void OnAbilityEnded(const FAbilityEndedData& AbilityEndedData);

public:
	FOnResolveUseCard OnResolveUseCard;
	FOnTryActivateEnemyAbility OnTryActivateEnemyAbility;
	FOnFinishActivationQueue OnFinishActivationQueue;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AActor> DummyActorClass;

private:
	UPROPERTY()
	TObjectPtr<AActor> DummyActor;
	
	/**
	 * Array지만 사실상 Queue의 작동 방식을 갖습니다.
	 * TQueue는 Dequeue할 때마다 값복사가 발생하기 때문에 TArray로 구현합니다.
	 */
	TArray<FAbilityActivationContext> PlayerAbilityActivationContexts;
	TArray<FAbilityActivationContext> EnemyAbilityActivationContexts;
	
	/** PlayerAbilityQueue가 작동 중인지를 표현하는 변수입니다. */
	uint8 bIsResolvingPlayerAbility : 1 = false;

	ETeamSide CurrentActivatorTeamSide = ETeamSide::None;
	
	TWeakObjectPtr<UAbilitySystemComponent> CurrentActivatorASC;
	uint8 bIsHandlingAbilityActivation : 1 = false;
	uint8 bPendingAbilitySucceeded : 1 = false;
	uint8 bPendingAbilityFailed : 1 = false;

	FDelegateHandle OnAbilityEndedDelegateHandle;

#if WITH_EDITOR
public:
	void AppendDebugSnapshot(FStringBuilderBase& Builder) const;
#endif
};
