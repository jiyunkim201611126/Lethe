// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectDelivery.h"
#include "EffectDelivery_Immediately.generated.h"

/**
 * 해당 구조체는 별다른 조건 없이, EffectSpec을 TargetASC에 즉시 적용합니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FEffectDelivery_Immediately : public FGameplayEffectDelivery
{
	GENERATED_BODY()
	
public:
	virtual void StartDelivery(const FEffectDeliveryContext& Context) const override;
};
