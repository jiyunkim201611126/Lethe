// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "UObject/Object.h"
#include "GameplayEffectApplier.generated.h"

class UGameplayAbility;
class UGameplayEffect;

/**
 * Effect 적용을 담당하는 구조체입니다.
 * 파생된 자식 구조체는 필요한 GameplayEffect 클래스와 함께 그에 관련된 멤버 변수가 선언 및 할당됩니다.
 */
UCLASS(Abstract, NotBlueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LETHE_API UGameplayEffectApplier : public UObject
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor) PURE_VIRTUAL(ULetheEffectApplier::ApplyEffect, );
	virtual bool TryMakeSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview = false) const PURE_VIRTUAL(ULetheEffectApplier::TryMakeSpecHandles, return false;);

	UFUNCTION(BlueprintCallable, Category = "Effect")
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const;
	
	TSubclassOf<UGameplayEffect> GetEffectClass() const;
	const FGameplayTag& GetEffectApplierTag() const;

	/** Ability 자체 Cost 외, Preview용 데이터 계산을 위한 GE 클래스를 반환해주는 함수입니다. */
	virtual TSubclassOf<UGameplayEffect> GetSourcePreviewEffectClass() const;
	
	/** Ability 자체 Cost 외, Effect 적용 시 Source에게 발생하는 Attribute 변화를 Preview로 표시할 때 사용하는 함수입니다. */
	virtual bool TryMakeSpecHandlesForSourcePreview(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const;

protected:
	void MakeEffectContextHandle(const UGameplayAbility* OwningAbility, FGameplayEffectContextHandle& OutHandle) const;
	bool TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag EffectApplierTag;
};
