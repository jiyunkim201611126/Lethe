// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameplayAbility.h"
#include "LetheMoveAbility.generated.h"

class ATile;
class UGameplayEffect;

/**
 * 타일 경로를 따라 캐릭터를 이동시키는 Ability입니다. 기존 GA_Move 블루프린트 그래프를 C++로 옮긴 것으로,
 * 블루프린트에는 Class Defaults 값만 남깁니다.
 *
 * 흐름: Payload 해석 → 발동 시점 재검증 → 논리 위치 확정(MapTileAndActor) → 걷기 시작 → 도착 대기 → 결과 통보 → EndAbility
 * AbilityResolver는 EndAbility를 '행동 완료'로 보므로, 종료는 몽타주가 아닌 캐릭터의 도착 신호로만 판단합니다.
 * 몽타주는 연출 전용이며, 끊기거나 비어 있어도 종료에 영향을 주지 않습니다.
 */
UCLASS()
class LETHE_API ULetheMoveAbility : public ULetheGameplayAbility
{
	GENERATED_BODY()

public:
	ULetheMoveAbility();

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~ End of UGameplayAbility Interface

private:
	/** AbilityResolver가 만든 UMoveAbilityPayload에서 경로를 꺼냅니다. 경로는 시작 타일을 포함합니다. */
	bool InitMovePath(const FGameplayEventData* TriggerEventData);

	/** 큐에서 꺼낸 시점에 다시 검증합니다. 예약 이후 목적지가 점유됐거나 이동력이 줄었을 수 있습니다. */
	bool CanMoveToTargetTile() const;

	/** 경로 중 플레이어에게 보이는 타일이 하나라도 있는지 반환합니다. */
	bool IsPathVisible() const;

	/** Ability에 지정된 몽타주가 있으면 그것을, 없으면 캐릭터의 MoveAnimation을 사용합니다. */
	UAnimMontage* SelectMoveMontage() const;

	/** 시작 타일과 목적지 사이의 거리만큼 MoveRange를 소모합니다. */
	void ApplyMoveCost() const;

	/** 이동 결과를 알립니다. 플레이어는 코스트·소음 적용 후 GameState에, 적은 StateTree에 알립니다. */
	void NotifyMoveResolved(const bool bSuccess);

	UFUNCTION()
	void OnMoveArrived(bool bTimedOut);

	void FinishMove(const bool bSuccess);

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	TSubclassOf<UGameplayEffect> MoveCostEffectClass;

	/** 비어 있으면 캐릭터의 MoveAnimation을 사용합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	TObjectPtr<UAnimMontage> MoveMontageOverride;

	/** 경로에 보이는 타일이 하나도 없을 때 걷기·몽타주 속도에 곱합니다. 보이지 않는 이동을 빠르게 넘기기 위한 값입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Move", meta = (ClampMin = "1.0"))
	float HiddenPathSpeedMultiplier = 5.f;

	/** 도착 신호가 오지 않을 때 강제로 종료하기까지의 실제 시간(초)입니다. 0 이하면 사용하지 않습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Move")
	float ArriveTimeoutSeconds = 15.f;

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<ATile>> MovePathTiles;

	UPROPERTY(Transient)
	TObjectPtr<ATile> MoveStartTile;

	UPROPERTY(Transient)
	TObjectPtr<ATile> MoveTargetTile;
};
