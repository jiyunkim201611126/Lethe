// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "LetheAbilitySystemGlobals.generated.h"

/**
 * GAS의 전역 설정 및 초기화 관리자 클래스입니다.
 * 현재 GameplayEffectContext 생성 방식을 커스텀한 상태입니다.
 */
UCLASS()
class LETHE_API ULetheAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
