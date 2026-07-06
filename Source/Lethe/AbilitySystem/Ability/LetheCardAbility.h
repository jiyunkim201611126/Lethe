// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/EffectDelivery/GameplayEffectDelivery.h"
#include "Lethe/AbilitySystem/EffectSpecBuilder/GameplayEffectSpecBuilder.h"
#include "StructUtils/InstancedStruct.h"
#include "LetheCardAbility.generated.h"

class UEffectTargetTileSelector;

USTRUCT(BlueprintType)
struct FEffectTargetMappingPolicy
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FGameplayTag MontageEventTag;

	/** 사용할 EffectSpecBuilder 태그 모음입니다. */
	UPROPERTY(EditDefaultsOnly)
	FGameplayTagContainer EffectSpecBuilderTags;

	/**
	 * TargetActorIndices에 'AllIndices'를 할당한다는 건, 캐싱된 모든 TargetActors를 대상으로 삼겠다는 의미입니다.
	 * 'AllIndices'이 들어간 이상 배열에는 1개의 요소만 있어야 합니다.
	 */
	static constexpr int32 AllIndices = -1;

	/**
	 * EffectSpecBuilderTags에 해당하는 EffectSpecBuilder가 만든 EffectSpec을 적용할 TargetActor의 Index 모음입니다.
	 * CachedTargetActors를 기준으로 수행되며, -1은 캐싱된 모든 TargetActor에게 적용하겠다는 의미입니다.
	 */
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

struct FEffectTargetMappingResolveResult
{
	TArray<FGameplayEffectSpecHandle> SourceSpecHandles;
	TMap<AActor*, TArray<FGameplayEffectSpecHandle>> TargetSpecHandlesByActor;
};

/**
 * 해당 프로젝트에서 Card는 Ability를 표현하는 수단이며, Ability는 해당 카드를 사용함으로 수행되는 캐릭터의 동작입니다.
 */
UCLASS()
class LETHE_API ULetheCardAbility : public ULetheGameplayAbility
{
	GENERATED_BODY()

public:
	ULetheCardAbility();

	UFUNCTION(BlueprintImplementableEvent)
	FText GetCardDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, const int32 InWeight) const;

    /** 카드 상세보기 시 설명용 텍스트에 넣을 값을 가져오는 함수입니다. */
	UFUNCTION(BlueprintPure, Category = "Effect")
	int32 GetEffectSpecBuilderValueForDescription(const FGameplayTag& EffectSpecBuilderTag, const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const;

	/**
	 * 시전 가능 범위에 해당하는 타일과 적용 후보 타일들을 가져옵니다.
	 * 여기서 OutTargetCandidateTiles는 마우스 위치가 범위를 벗어난 경우 비어있을 수 있습니다.
	 */
	void GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles) const;

	/** 시전 시 적용될 대상이 존재하는 타일들을 가져옵니다. */
	void GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const;

	/** Ability 발동 시 어떤 효과가 발생하는지 미리보기용 데이터를 가져오는 함수입니다. */
	bool TryGetCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const;
	bool TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<AActor*>& TargetActors, FGameplayEffectPreviewData& OutPreviewData) const;

	//~ Begin UGameplayAbility Interface
	virtual void CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility) override;
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CommitAbilityCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility Interface

	UFUNCTION(BlueprintImplementableEvent, meta = (ToolTip = "Ability가 발동되어 실제로 동작이 트리거됐을 때 호출됩니다. Effect 적용 시점이 아닌, Ability의 동작이 기준입니다."))
	void OnEffectTriggered(const FGameplayTag& MontageEventTag, const TArray<AActor*>& TargetActors);

private:
	bool TryValidateAndCommitActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	void ActiveFailed();

	void ResolveEffectTargetMappingPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, UAbilitySystemComponent* SourceASC, const TArray<AActor*>& CandidateTargetActors, FEffectTargetMappingResolveResult& OutResult) const;

	void GetTargetActorsByPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, const TArray<AActor*>& CandidateTargetActors, TArray<AActor*>& OutTargetActors) const;
	void GetEffectSpecBuildersByPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, TArray<const FGameplayEffectSpecBuilder*>& OutEffectSpecBuilders) const;

	void ResetCachedValues();

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

	UFUNCTION(BlueprintPure, Category = "Effect")
	FText GetWeightDescription(const int32 Weight) const;

	bool TryGetGameplayEffectPreviewData(UAbilitySystemComponent* PreviewTargetASC, const TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const;

	/** AnimNotify를 통해 이벤트를 받았을 때 호출되는 함수입니다. */
	UFUNCTION()
	void OnEventReceived(FGameplayEventData InPayload);

	/** Effect를 지정된 방식으로 TargetActor에게 전달을 시작합니다. */
	void StartDeliveryEffects(AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& SpecHandles) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AbilityAnimMontage;

	/**
	 * TargetTile 지정을 수행하는 객체입니다.
	 * 할당하지 않으면 자동으로 마우스 위치의 타일 하나만 TargetTile로 지정됩니다.
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Effect")
	TObjectPtr<UEffectTargetTileSelector> EffectTargetTileSelector;

	/** Composite 패턴으로 조합해 사용하며, Effect 적용에 필요한 GameplayEffectSpec을 생성합니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FGameplayEffectSpecBuilder>> EffectSpecBuilders;

	/** 갖고 있는 EffectSpecBuilders가 만든 EffectSpec을 CachedTargetActors 중 누구에게 적용할지 결정하는 정책입니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<FEffectTargetMappingPolicy> EffectTargetMappingPolicies;

	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ExcludeBaseStruct))
	TInstancedStruct<FGameplayEffectDelivery> EffectDelivery;

	TArray<TWeakObjectPtr<AActor>> CachedTargetActors;

private:
	TWeakObjectPtr<const ATile> CachedCenterTargetTile;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
