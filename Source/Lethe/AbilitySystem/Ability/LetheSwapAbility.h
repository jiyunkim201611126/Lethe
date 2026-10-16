// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheMoveAbilityBase.h"
#include "LetheSwapAbility.generated.h"

/**
 * 아군 캐릭터와 자리를 맞바꾸는 Ability입니다. 기존 GA_Swap 블루프린트 그래프를 C++로 옮긴 것으로,
 * 블루프린트에는 Class Defaults 값만 남깁니다.
 *
 * 아바타는 경로를 정방향으로, 상대(Payload.OptionalObject2)는 같은 경로를 역방향으로 걷습니다.
 * 두 캐릭터가 모두 도착해야 종료되며, 코스트와 소음은 양쪽 모두에 적용합니다.
 */
UCLASS()
class LETHE_API ULetheSwapAbility : public ULetheMoveAbilityBase
{
	GENERATED_BODY()

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	//~ End of UGameplayAbility Interface

	//~ Begin ULetheMoveAbilityBase Interface
	/** 성공 시 두 캐릭터에 코스트·소음을 적용한 뒤 GameState에 알립니다. 스왑 상대의 예약 이동 갱신은 PlayerAbilityRequestComponent가 처리합니다. */
	virtual void NotifyMoveResolved(const bool bSuccess) override;
	//~ End of ULetheMoveAbilityBase Interface

private:
	/** Payload.OptionalObject2에서 스왑 상대를 꺼냅니다. */
	bool InitSwapTarget(const FGameplayEventData* TriggerEventData);

	/** 큐에서 꺼낸 시점에 다시 검증합니다. 예약 이후 상대가 자리를 떠났거나 이동력이 줄었을 수 있습니다. */
	bool CanSwap() const;

private:
	UPROPERTY(Transient)
	TObjectPtr<ALetheCharacterBase> SwapTargetCharacter;
};
