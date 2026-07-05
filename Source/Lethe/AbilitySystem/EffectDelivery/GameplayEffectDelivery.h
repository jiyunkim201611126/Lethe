// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectDelivery.generated.h"

class UGameplayAbility;
class UAbilitySystemComponent;
struct FGameplayEffectSpecHandle;

USTRUCT()
struct FEffectDeliveryContext
{
	GENERATED_BODY()

	TArray<FGameplayEffectSpecHandle> EffectSpecHandles;
	TWeakObjectPtr<const UGameplayAbility> OwnerAbility;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;
	TWeakObjectPtr<UAbilitySystemComponent> TargetASC;

	bool IsValid() const;
};

/**
 * EffectSpec을 어떤 방식으로 전달할지 결정하는 구조체입니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FGameplayEffectDelivery
{
	GENERATED_BODY()

	virtual ~FGameplayEffectDelivery() = default;

public:
	virtual void StartDelivery(const FEffectDeliveryContext& Context) const;
};
