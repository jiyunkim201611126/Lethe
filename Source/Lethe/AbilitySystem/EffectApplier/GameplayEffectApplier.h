// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "UObject/Object.h"
#include "GameplayEffectApplier.generated.h"

class UGameplayEffect;
class UGameplayAbility;

/**
 * Effect 적용을 담당하는 클래스입니다.
 * 파생된 자식 클래스는 필요한 GameplayEffect 클래스와 함께 그에 관련된 멤버 변수가 선언 및 할당됩니다.
 */
UCLASS(NotBlueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LETHE_API UGameplayEffectApplier : public UObject
{
	GENERATED_BODY()

public:
	virtual void ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor) PURE_VIRTUAL(UGameplayEffectApplier::ApplyEffect, );
	virtual void CancelAbility();
	virtual void EndAbility();
	
	void MakeEffectContextHandle(const UGameplayAbility* OwningAbility);

	FGameplayEffectContextHandle GetEffectContextHandle() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	FGameplayEffectContextHandle EffectContextHandle;
};
