// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "ExecCalc_Damage.generated.h"

/**
 * MMC와 마찬가지로 GameplayEffect가 적용될 때 Attribute를 통해 어떠한 계산 결과를 얻는 데에 사용하는 클래스입니다.
 * 한 대상의 여러 Attribute를 변경할 수 있으며, Modifier만으로는 구현이 어려운 커스텀 로직을 작성하는 데에 사용합니다.
 * 단, MMC는 블루프린트로 구현할 수 있는 반면 ExecCalc는 코드상으로만 구현이 가능합니다.
 * 예) 거리 반비례 데미지 구현, 여러 스탯 동시에 증가 등
 *
 * 해당 클래스는 통상적인 데미지를 계산합니다.
 */
UCLASS()
class LETHE_API UExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UExecCalc_Damage();

	//~ Begin GameplayEffectExecutionCalculation Interface
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
	//~ End GameplayEffectExecutionCalculation Interface
};
