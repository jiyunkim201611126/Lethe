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
USTRUCT(BlueprintType)
struct LETHE_API FGameplayEffectApplier
{
	GENERATED_BODY()

public:
	virtual ~FGameplayEffectApplier() = default;

	virtual void ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor);
	virtual void CancelAbility();
	virtual void EndAbility();
	
	void MakeEffectContextHandle(const UGameplayAbility* OwningAbility);

	FGameplayEffectContextHandle GetEffectContextHandle() const;

	virtual FText GetDescriptionText(const int32 InLevel) const;

	TSubclassOf<UGameplayEffect> GetEffectClass() const;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	FGameplayEffectContextHandle EffectContextHandle;
};
