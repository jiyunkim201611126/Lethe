// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectSpecBuilder.generated.h"

class UGameplayEffect;

/**
 * EffectSpec 생성을 담당하는 구조체입니다.
 * 파생된 자식 구조체는 필요한 GameplayEffect 클래스와 함께 그에 관련된 멤버 변수가 선언 및 할당됩니다.
 */
USTRUCT(BlueprintType)
struct LETHE_API FGameplayEffectSpecBuilder
{
	GENERATED_BODY()

	virtual ~FGameplayEffectSpecBuilder() = default;

	virtual bool TryBuildSourceEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const;
	virtual bool TryBuildTargetEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const;
	
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const;
	
	const FGameplayTag& GetEffectSpecBuilderTag() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	/** EffectTargetMappingPolicy가 사용하는 태그입니다. */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag EffectSpecBuilderTag;
};
