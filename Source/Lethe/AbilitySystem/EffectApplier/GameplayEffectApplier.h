// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "UObject/Object.h"
#include "GameplayEffectApplier.generated.h"

class UGameplayAbility;
class UGameplayEffect;

UENUM()
enum class EEffectPreviewTarget : uint8
{
	Source,
	Target
};

USTRUCT()
struct FPreviewEffectSpecContext
{
	GENERATED_BODY()
	
	EEffectPreviewTarget Target = EEffectPreviewTarget::Target;

	UPROPERTY()
	TSubclassOf<UGameplayEffect> EffectClass;
	
	TArray<FGameplayEffectSpecHandle> SpecHandles;
};

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
	
	virtual bool TryMakeSpecHandles(const UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const PURE_VIRTUAL(ULetheEffectApplier::TryMakeSpecHandles, return false;);

	bool TryMakeSpecHandlesWithContextHandle(const UGameplayAbility* OwningAbility, TArray<FGameplayEffectSpecHandle>& OutSpecHandles);
	void MakeEffectContextHandle(const UGameplayAbility* OwningAbility);
	
	FGameplayEffectContextHandle GetEffectContextHandle() const;

	// Preview 데이터를 만들기 위한 Spec Context를 생성합니다.
	// 기본 구현은 Target을 대상으로 하는 Preview 데이터만 생성하며, Source를 대상으로 하는 데이터가 필요한 경우 자식에서 오버라이드해 구현합니다.
	virtual bool TryBuildPreviewSpecContexts(const UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FPreviewEffectSpecContext>& OutPreviewEffectSpecContexts) const;
	
	UFUNCTION(BlueprintCallable, Category = "Effect")
	virtual int32 GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const;
	
protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> EffectClass;

	FGameplayEffectContextHandle EffectContextHandle;
};
