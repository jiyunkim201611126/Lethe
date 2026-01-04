// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Lethe/AbilitySystem/EffectApplier/GameplayEffectApplier.h"
#include "LetheGameplayAbility.generated.h"

UCLASS()
class LETHE_API ULetheGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	/**
	 * Ability가 소유하고 있는 EffectApplier를 모두 순회하며 TargetActor에게 Effect를 부여하는 함수입니다.
	 * 갖고 있는 Effect를 각각 다른 타이밍에 부여하고 싶다면 자식 클래스로 캐스팅해 ApplyEffect를 직접 호출해 사용합니다.
	 */
	UFUNCTION(BlueprintCallable, Category = "Effect")
	void ApplyAllEffects(AActor* TargetActor);

protected:
	template<typename T>
	T* GetEffectApplier() const
	{
		// T가 UGameplayEffectApplier 상속받지 않는 경우 오류를 발생시키는 구문입니다.
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
	 * 반드시 Ability가 소유하고 있는 GameplayEffectApplier를 사용해야 합니다.
	 * 블루프린트에선 템플릿 함수를 지원하지 않기 때문에 Class를 매개변수로 받아 비슷한 동작을 하도록 구현합니다.
	 */ 
	UFUNCTION(BlueprintPure, Category = "Effect")
	FGameplayEffectContextHandle GetContextHandle(const TSubclassOf<UGameplayEffectApplier>& ApplierClass) const;

public:
	// 식별자 역할의 Ability의 전용 태그입니다.
	// 플레이어 캐릭터가 사용하는 Ability는 반드시 Tag 하나와 1:1 대응합니다.
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTag AbilityTag;

protected:
	// 할당과 동시에 객체화되는 멤버변수입니다.
	// Composite 패턴으로 조합해 사용할 수 있으며, 클래스 내부의 ApplyEffect나 ApplyAllEffects를 호출해 사용합니다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Effect")
	TArray<TObjectPtr<UGameplayEffectApplier>> EffectAppliers;

protected:
	//~ Begin UGameplayAbility Interface
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End UGameplayAbility Interface

#if WITH_EDITOR
	// AbilityTag로 할당한 Ability 전용 태그가 AssetTag(AbilityTags)에도 자동으로 추가되도록 해주는 함수들입니다.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostInitProperties() override;

private:
	void SyncAbilityTagToAssetTags();
#endif
};
