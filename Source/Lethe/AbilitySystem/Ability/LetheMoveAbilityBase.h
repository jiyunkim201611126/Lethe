// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameplayAbility.h"
#include "LetheMoveAbilityBase.generated.h"

class ALetheCharacterBase;
class ATile;
class UGameplayEffect;

/**
 * 타일 경로를 걷는 비카드 Ability(GA_Move, GA_Swap)의 공통 부모입니다.
 * 경로 Payload 해석, 이동 코스트, 몽타주 재생, 도착 대기를 모아 두고, 파생 클래스는 검증·논리 위치 확정·결과 통보만 정의합니다.
 *
 * AbilityResolver는 EndAbility를 '행동 완료'로 보므로, 종료는 몽타주가 아닌 캐릭터들의 도착 신호로만 판단합니다.
 * 몽타주는 연출 전용이며, 끊기거나 비어 있어도 종료에 영향을 주지 않습니다.
 */
UCLASS(Abstract)
class LETHE_API ULetheMoveAbilityBase : public ULetheGameplayAbility
{
	GENERATED_BODY()

public:
	ULetheMoveAbilityBase();

protected:
	//~ Begin UGameplayAbility Interface
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility Interface

	/** AbilityResolver가 만든 UMoveAbilityPayload에서 경로를 꺼냅니다. 경로는 시작 타일을 포함합니다. */
	bool InitMovePath(const FGameplayEventData* TriggerEventData);

	/** 경로에 보이는 타일이 하나도 없으면 HiddenPathSpeedMultiplier를, 아니면 1을 반환합니다. */
	float GetPathSpeedMultiplier() const;

	/** 시작 타일과 목적지 사이의 최단 거리입니다. 경로 길이와 다를 수 있습니다. */
	int32 GetMoveDistance() const;

	/** MovePathTiles를 따라(bReversePath면 역순으로) 캐릭터를 걷게 합니다. */
	void StartWalking(ALetheCharacterBase* MovingCharacter, const bool bReversePath, const float SpeedMultiplier) const;

	/**
	 * 캐릭터의 이동 몽타주를 재생합니다.
	 * 아바타의 몽타주는 Task로 재생해 Ability 종료 시 함께 멈추고, 다른 캐릭터의 몽타주는 EndAbility에서 직접 멈춥니다.
	 */
	void PlayMoveMontage(ALetheCharacterBase* MovingCharacter, const float PlayRate);

	/**
	 * MovingCharacters가 모두 도착하면 FinishMove(true)로 종료합니다.
	 * 이미 도착해 있으면 즉시 종료되므로, 걷기·몽타주를 모두 시작한 뒤 마지막에 호출해야 합니다.
	 */
	void WaitForArrival(const TArray<ALetheCharacterBase*>& MovingCharacters);

	/** 최단 거리만큼 TargetASC의 MoveRange를 소모합니다(기존 GA_Move/GA_Swap 동작 유지). */
	void ApplyMoveCost(UAbilitySystemComponent* TargetASC) const;

	/** 결과를 알린 뒤 Ability를 종료합니다. */
	void FinishMove(const bool bSuccess);

	/** 이동 결과를 알립니다. 실패해도 호출되므로, 코스트·소음은 성공한 경우에만 적용해야 합니다. */
	virtual void NotifyMoveResolved(const bool bSuccess) PURE_VIRTUAL(ULetheMoveAbilityBase::NotifyMoveResolved, );

private:
	/** Ability에 지정된 몽타주가 있으면 그것을, 없으면 캐릭터의 MoveAnimation을 사용합니다. */
	UAnimMontage* SelectMoveMontage(ALetheCharacterBase* MovingCharacter) const;

	UFUNCTION()
	void OnMoveArrived(bool bTimedOut);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	TSubclassOf<UGameplayEffect> MoveCostEffectClass;

	/** 비어 있으면 각 캐릭터의 MoveAnimation을 사용합니다. 지정하면 이 Ability로 움직이는 모든 캐릭터가 이 몽타주를 재생합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	TObjectPtr<UAnimMontage> MoveMontageOverride;

	/** 경로에 보이는 타일이 하나도 없을 때 걷기·몽타주 속도에 곱합니다. 보이지 않는 이동을 빠르게 넘기기 위한 값입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Move", meta = (ClampMin = "1.0"))
	float HiddenPathSpeedMultiplier = 5.f;

	/** 도착 신호가 오지 않을 때 강제로 종료하기까지의 실제 시간(초)입니다. 0 이하면 사용하지 않습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	float ArriveTimeoutSeconds = 15.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<ATile>> MovePathTiles;

	UPROPERTY(Transient)
	TObjectPtr<ATile> MoveStartTile;

	UPROPERTY(Transient)
	TObjectPtr<ATile> MoveTargetTile;

private:
	/** 아바타가 아닌 캐릭터에게 재생한 몽타주입니다. Ability와 묶이지 않으므로 EndAbility에서 직접 멈춥니다. */
	TArray<TPair<TWeakObjectPtr<ALetheCharacterBase>, TWeakObjectPtr<UAnimMontage>>> ExternalMoveMontages;
};
