// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectSpecBuilder.generated.h"

class UGameplayAbility;
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

	/**
	 * Preview 용도의 Spec을 원하는 경우 마지막 bPreview 인자를 true로 하여 외부에서 호출합니다.
	 * 실제 적용을 위해 호출하는 경우 구현체에 따라 Cost 소비 등의 준비 작업이 들어갈 수 있습니다.
	 */
	virtual bool TryBuildEffectSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview = false) const;
	
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const;
	
	TSubclassOf<UGameplayEffect> GetEffectClass() const;
	const FGameplayTag& GetEffectSpecBuilderTag() const;

	/** Ability 자체 Cost 외, Preview용 데이터 계산을 위한 GE 클래스를 반환해주는 함수입니다. */
	virtual TSubclassOf<UGameplayEffect> GetSourcePreviewEffectClass() const;
	
	/** Ability 자체 Cost 외, Effect 적용 시 Source에게 발생하는 Attribute 변화를 Preview로 표시할 때 사용하는 함수입니다. */
	virtual bool TryBuildSourcePreviewSpecHandles(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	/** EffectTargetMappingPolicy가 사용하는 태그입니다. */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag EffectSpecBuilderTag;
};
