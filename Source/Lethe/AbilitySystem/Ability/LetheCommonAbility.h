// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheCardAbility.h"
#include "Lethe/AbilitySystem/EffectDelivery/GameplayEffectDelivery.h"
#include "StructUtils/InstancedStruct.h"
#include "LetheCommonAbility.generated.h"

USTRUCT(BlueprintType)
struct FEffectTargetMappingPolicy
{
	GENERATED_BODY()

	FEffectTargetMappingPolicy();

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag MontageEventTag;

	/** 사용할 EffectSpecBuilder 태그 모음입니다. */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer EffectSpecBuilderTags;

	/** 해당 플래그가 true면 모든 그룹, 모든 타겟을 대상으로 Effect를 적용합니다. */
	UPROPERTY(EditDefaultsOnly)
	bool bApplyToAllTargets = true;

	/** 적용할 대상 그룹 태그 모음입니다. */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bApplyToAllTargets"))
	FGameplayTagContainer TargetGroupTags;
};

struct FEffectTargetMappingResolveResult
{
	TArray<FGameplayEffectSpecHandle> SourceSpecHandles;

	/**
	 * Key는 Effect를 적용할 액터, Value는 적용될 Effect들입니다.
	 * 로컬 변수로 잠깐 생성되는 구조체기 때문에 오래 들고 있을 필요가 없어 UPROPERTY는 붙이지 않습니다.
	 */
	TMap<AActor*, TArray<FGameplayEffectSpecHandle>> TargetSpecHandlesByActor;
};

/**
 * 대부분의 일반적인 Ability가 이 클래스를 상속받습니다.
 *
 * 정책에 따라 TargetActors를 선정, 정책에 따라 EffectSpec 생성, 지정된 방식으로 TargetActors에게 EffectSpec을 적용하는 로직을 갖습니다.
 */
UCLASS()
class LETHE_API ULetheCommonAbility : public ULetheCardAbility
{
	GENERATED_BODY()

public:
	ULetheCommonAbility();

	virtual bool TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<FTargetSelectionResult>& TargetSelectionResults, FGameplayEffectPreviewData& OutPreviewData) const override;
	virtual void GetCardDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, FText& OutDescription) const override;
	
protected:
	virtual void RegisterAbilityEventTasks() override;
	virtual void HandleAbilityEvent(const FGameplayEventData& InPayload) override;

private:
	/** Effect를 지정된 방식으로 TargetActor에게 전달을 시작합니다. */
	void StartDeliveryEffects(AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& SpecHandles) const;
	
	/**
	 * Policy를 기반으로 어떤 대상에게 어떤 Effect를 적용할지를 취합합니다.
	 * MontageEvent에 해당하는 Policy만 따로 집계해야 하므로, 한 번에 뭉탱이로 계산하지 않고 Policy별로 따로 집계합니다.
	 */
	void ResolveEffectTargetMappingPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, UAbilitySystemComponent* SourceASC, const TArray<FTargetSelectionResult>& TargetSelectionResults, FEffectTargetMappingResolveResult& OutResult) const;

	void GetEffectSpecBuildersByPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, TArray<const FGameplayEffectSpecBuilder*>& OutEffectSpecBuilders) const;
	
	template<typename T>
	const T* GetEffectSpecBuilder() const
	{
		static_assert(TIsDerivedFrom<T, FGameplayEffectSpecBuilder>::IsDerived, "T는 반드시 FGameplayEffectSpecBuilder를 상속받아야 합니다.");

		for (const auto& Builder : EffectSpecBuilders)
		{
			if (const T* TypedBuilder = Builder.GetPtr<T>())
			{
				return TypedBuilder;
			}
		}
		return nullptr;
	}
	
protected:
	/** Composite 패턴으로 조합해 사용하며, Effect 적용에 필요한 GameplayEffectSpec을 생성합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FGameplayEffectSpecBuilder>> EffectSpecBuilders;

	/** EffectSpecBuilders가 생성한 EffectSpec을 어떤 TargetGroup에 적용할지 결정하는 정책입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<FEffectTargetMappingPolicy> EffectTargetMappingPolicies;

	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ExcludeBaseStruct))
	TInstancedStruct<FGameplayEffectDelivery> EffectDelivery;
};
