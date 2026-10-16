// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheMoveAbilityBase.h"
#include "LetheMoveAbility.generated.h"

/**
 * 타일 경로를 따라 캐릭터를 이동시키는 Ability입니다. 기존 GA_Move 블루프린트 그래프를 C++로 옮긴 것으로,
 * 블루프린트에는 Class Defaults 값만 남깁니다.
 *
 * 흐름: Payload 해석 → 발동 시점 재검증 → 논리 위치 확정(MapTileAndActor) → 걷기 시작 → 도착 대기 → 결과 통보 → EndAbility
 */
UCLASS()
class LETHE_API ULetheMoveAbility : public ULetheMoveAbilityBase
{
	GENERATED_BODY()

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~ End of UGameplayAbility Interface

	//~ Begin ULetheMoveAbilityBase Interface
	/** 플레이어는 코스트·소음 적용 후 GameState에, 적은 StateTree에 알립니다. */
	virtual void NotifyMoveResolved(const bool bSuccess) override;
	//~ End of ULetheMoveAbilityBase Interface

private:
	/** 큐에서 꺼낸 시점에 다시 검증합니다. 예약 이후 목적지가 점유됐거나 이동력이 줄었을 수 있습니다. */
	bool CanMoveToTargetTile() const;
};
