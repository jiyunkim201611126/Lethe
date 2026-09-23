// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitMoveArrived.generated.h"

class ALetheCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWaitMoveArrivedDelegate, bool, bTimedOut);

/**
 * 지정한 캐릭터들이 모두 경로 이동을 끝낼 때까지 기다리는 Task입니다.
 * 이동 Ability의 종료 시점을 몽타주가 아닌 '실제 도착'으로 맞추기 위해 사용합니다.
 *
 * 타임아웃은 도착 신호가 끝내 오지 않을 때 턴이 멈추지 않도록 하는 안전장치입니다.
 * 추후 시간 정지(전역 TimeDilation) 연출과 함께 동작해야 하므로, 월드 시간이 아닌 실제 프레임 시간으로 누적합니다.
 */
UCLASS()
class LETHE_API UAbilityTask_WaitMoveArrived : public UAbilityTask
{
	GENERATED_BODY()

public:
	UAbilityTask_WaitMoveArrived(const FObjectInitializer& ObjectInitializer);

	/**
	 * MovingCharacters가 모두 이동을 끝내면 OnArrived를 호출합니다.
	 * Task 활성화 시점에 이미 이동 중이 아닌 캐릭터는 도착한 것으로 봅니다.
	 * TimeoutSeconds가 0 이하면 타임아웃을 사용하지 않습니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
	static UAbilityTask_WaitMoveArrived* WaitMoveArrived(UGameplayAbility* OwningAbility, const TArray<ALetheCharacterBase*>& MovingCharacters, const float TimeoutSeconds = 15.f);

	//~ Begin UGameplayTask Interface
	virtual void Activate() override;
	virtual void TickTask(float DeltaTime) override;
	virtual void OnDestroy(bool bInOwnerFinished) override;
	//~ End of UGameplayTask Interface

	UPROPERTY(BlueprintAssignable)
	FWaitMoveArrivedDelegate OnArrived;

private:
	void HandleMoveFinished(TWeakObjectPtr<ALetheCharacterBase> MovedCharacter);
	void CompleteIfAllArrived();
	void Complete(const bool bTimedOut);
	void UnbindAll();

	/** 아직 도착하지 않은 캐릭터들입니다. 이동 중 파괴된 캐릭터는 Tick에서 걸러냅니다. */
	TArray<TWeakObjectPtr<ALetheCharacterBase>> PendingCharacters;

	TArray<TPair<TWeakObjectPtr<ALetheCharacterBase>, FDelegateHandle>> BoundHandles;

	float TimeoutSeconds = 0.f;
	float ElapsedRealSeconds = 0.f;
	uint8 bCompleted : 1 = false;
};
