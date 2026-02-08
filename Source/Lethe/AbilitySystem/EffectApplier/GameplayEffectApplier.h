// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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
	virtual void CancelAbility();
	virtual void EndAbility();
	virtual bool TryMakeSpecHandles(const UAbilitySystemComponent* SourceASC, const UGameplayAbility* OwningAbility, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const PURE_VIRTUAL(ULetheEffectApplier::TryMakeSpecHandles, return false;);
	virtual FText GetDescriptionText(const int32 InLevel) const;

	bool TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, TArray<FGameplayEffectSpecHandle>& OutSpecHandles);
	void MakeEffectContextHandle(const UGameplayAbility* OwningAbility);
	
	TSubclassOf<UGameplayEffect> GetEffectClass() const;
	FGameplayEffectContextHandle GetEffectContextHandle() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	FGameplayEffectContextHandle EffectContextHandle;
};
