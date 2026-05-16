// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/EffectApplier/GameplayEffectApplier.h"
#include "LetheCardAbility.generated.h"

USTRUCT(BlueprintType)
struct FEffectApplyPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag MontageEventTag;

	/** 적용할 EffectApplier 태그 모음입니다. */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer EffectApplierTags;

	/**
	 * TargetActorIndices에 'AllIndices'를 할당한다는 건, 캐싱된 모든 TargetActors를 대상으로 삼겠다는 의미입니다.
	 * 'AllIndices'이 들어간 이상 배열에는 1개의 요소만 있어야 합니다.
	 */
	static constexpr int32 AllIndices = -1;

	/** TargetActor에게 적용할 EffectApplier Index입니다. */
	UPROPERTY(EditDefaultsOnly)
	TArray<int32> TargetActorIndices = { AllIndices };
};

/** TryGetEffectsForSourceAndTargetPreviewData 호출 시 사용하는 구조체입니다. */
struct FGameplayEffectPreviewData
{
	TMap<FGameplayAttribute, float> SourcePreviewData;
	TMap<UAbilitySystemComponent*, TMap<FGameplayAttribute, float>> TargetPreviewData;

	bool IsEmpty() const
	{
		return SourcePreviewData.IsEmpty() && TargetPreviewData.IsEmpty();
	}
};

/**
 * 해당 프로젝트에서 Card는 Ability를 표현하는 UMG 수단이며, Ability는 해당 카드를 사용함으로 수행되는 캐릭터의 동작입니다.
 */
UCLASS()
class LETHE_API ULetheCardAbility : public ULetheGameplayAbility
{
	GENERATED_BODY()

public:
	ULetheCardAbility();
	
	UFUNCTION(BlueprintImplementableEvent)
	FText GetCardDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, const int32 InWeight) const;

	/** Ability 발동 시 어떤 효과가 발생하는지 미리보기용 데이터를 가져오는 함수입니다. */
	bool TryGetCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const;
	bool TryGetEffectsForSourcePreviewData(UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutPreviewData) const;
	bool TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<AActor*>& TargetActors, FGameplayEffectPreviewData& OutPreviewData) const;

protected:
	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility Interface

	UFUNCTION(BlueprintImplementableEvent)
	void OnApplyEffect(const FGameplayTag& MontageEventTag, const TArray<AActor*>& TargetActors);
	
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

	UFUNCTION(BlueprintPure, Category = "Effect")
	FText GetWeightDescription(const int32 Weight) const;

private:
	bool TryGetGameplayEffectPreviewData(UAbilitySystemComponent* PreviewTargetASC, const TSubclassOf<UGameplayEffect>& EffectClass, TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const;

	/** AnimNotify를 통해 이벤트를 받았을 때 호출되는 함수입니다. */
	UFUNCTION()
	void OnEventReceived(FGameplayEventData Payload);

	bool TryValidateAndCommitActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	void GetTargetActorsByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, const TArray<AActor*>& SourceTargetActors, TArray<AActor*>& OutTargetActors) const;
	void ApplyEffectsByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, AActor* TargetActor);
	void GetEffectAppliersByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, TArray<UGameplayEffectApplier*>& OutEffectAppliers) const;
	void ActiveFailed();

	void ResetCachedValues();
	
protected:
	/** Composite 패턴으로 조합해 사용할 수 있으며, 클래스의 ApplyEffect를 직접 호출하거나 Ability의 ApplyAllEffects를 호출해 사용합니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Effect")
	TArray<TObjectPtr<UGameplayEffectApplier>> EffectAppliers;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<FEffectApplyPolicy> EffectApplyPolicies;
	
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AbilityAnimMontage;

private:
	TWeakObjectPtr<const ATile> CachedCenterTargetTile;
	TArray<TWeakObjectPtr<AActor>> CachedTargetActors;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
