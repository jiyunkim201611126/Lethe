// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/EffectApplier/GameplayEffectApplier.h"
#include "LetheCardAbility.generated.h"

/**
 * 해당 프로젝트에서 Card는 Ability를 표현하는 UMG 수단이며, Ability는 해당 카드를 사용함으로 수행되는 캐릭터의 동작입니다.
 */
UCLASS()
class LETHE_API ULetheCardAbility : public ULetheGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * Card가 소유하고 있는 EffectApplier를 모두 순회하며 TargetActor에게 Effect를 부여하는 함수입니다.
	 * 갖고 있는 Effect를 각각 다른 타이밍에 부여하고 싶다면 인덱스로 접근해 호출합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	void ApplyAllEffects(AActor* TargetActor);

	// Card에 대한 설명을 반환하는 함수로, 갖고 있는 EffectAppliers를 순회하며 설명을 가져옵니다.
	UFUNCTION(BlueprintImplementableEvent)
	FText GetCardDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const;

	bool TryGetAbilityCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const;
	bool TryGetAbilityEffectsForSourcePreviewData(UAbilitySystemComponent* SourceASC, const UAbilitySystemComponent* TargetASC, TMap<FGameplayAttribute, float>& OutPreviewData) const;
	bool TryGetAbilityEffectsForTargetPreviewData(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TMap<FGameplayAttribute, float>& OutPreviewData) const;

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility Interface
	
	template<typename T>
	T* GetEffectApplier()
	{
		static_assert(TIsDerivedFrom<T, UGameplayEffectApplier>::IsDerived, "T는 반드시 UGameplayEffectApplier를 상속받아야 합니다.");

		for (UGameplayEffectApplier* Applier : EffectAppliers)
		{
			if (Applier && Applier->IsA<T>())
			{
				return Cast<T>(Applier);
			}
		}
		return nullptr;
	}
	
	/**
	 * 매개변수로 들어온 GameplayEffectApplier 클래스가 갖고 있는 GameplayEffectContextHandle을 가져오는 함수입니다.
	 * 반드시 Card가 소유하고 있는 GameplayEffectApplier를 사용해야 합니다.
	 */ 
	UFUNCTION(BlueprintPure, Category = "Effect")
	FGameplayEffectContextHandle GetContextHandle(const TSubclassOf<UGameplayEffectApplier>& ApplierClass) const;

	UFUNCTION(BlueprintPure, Category = "Effect")
	FText GetRangeDescription() const;

private:
	bool TryGetGameplayEffectPreviewData(UAbilitySystemComponent* TargetASC, const TSubclassOf<UGameplayEffect>& EffectClass, TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const;

	UFUNCTION()
	void OnEventReceived(FGameplayEventData Payload);

	void ActiveFailed();
	
protected:
	// Composite 패턴으로 조합해 사용할 수 있으며, 구조체 내부의 ApplyEffect나 Ability의 ApplyAllEffects를 호출해 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Effect")
	TArray<TObjectPtr<UGameplayEffectApplier>> EffectAppliers;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AbilityAnimMontage;

private:
	TWeakObjectPtr<AActor> CachedTargetActor;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
