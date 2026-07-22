// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/EffectTargetTileSelector/EffectTargetTileSelector.h"
#include "Lethe/AbilitySystem/EffectSpecBuilder/GameplayEffectSpecBuilder.h"
#include "Lethe/Data/AbilityActivationData.h"
#include "StructUtils/InstancedStruct.h"
#include "LetheCardAbility.generated.h"

/**
 * TryGetEffectsForSourceAndTargetPreviewData 호출 시 사용하는 구조체입니다.
 * 미리보기 작동 시 Source와 Target에게 어떤 변화량이 있을지 전달하는 데에 사용합니다.
 */
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
 * 해당 프로젝트에서 Card는 Ability를 표현하는 수단이며, Ability는 해당 카드를 사용함으로 수행되는 캐릭터의 동작입니다.
 *
 * 미리보기 데이터 추출 및 반환, 애니메이션 재생, TargetTile 선택 등의 로직을 포함합니다. 
 */
UCLASS()
class LETHE_API ULetheCardAbility : public ULetheGameplayAbility
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent)
	FText GetCardDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, const int32 InWeight) const;
	
	/**
	 * 시전 가능 범위에 해당하는 타일과 적용 후보 타일들을 가져옵니다.
	 * 여기서 Context의 OutTargetTileResults는 마우스 위치가 범위를 벗어난 경우 비어있을 수 있습니다.
	 */
	void GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const;

	/**
	 * 시전 시 적용될 대상이 존재하는 타일들을 가져옵니다.
	 * 외부에서 Combat Target 유무를 판단하는 경우가 있기 때문에, Ability가 직접 해당 함수를 호출해 Target을 수집하는 경우 중복 연산이 발생합니다.
	 * 따라서 외부에서 Combat Target 유무 판단을 위해 호출한 데이터를 그대로 재활용하는 방식으로 구현되었습니다.
	 */
	void GetTargetTiles(FEffectTargetTileSelectorContext& Context) const;

	/** Ability 발동 시 어떤 효과가 발생하는지 미리보기용 데이터를 가져오는 함수입니다. */
	bool TryGetCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const;
	virtual bool TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<FTargetSelectResult>& TargetSelectResults, FGameplayEffectPreviewData& OutPreviewData) const;

	//~ Begin UGameplayAbility Interface
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	virtual bool CommitAbilityCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	//~ End of UGameplayAbility Interface

	/** EffectSpec으로부터 미리보기 데이터를 추출합니다. */
	bool TryGetGameplayEffectPreviewData(UAbilitySystemComponent* PreviewTargetASC, const TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const;

	/** Ability 발동 후 AnimNotify를 통해 이벤트를 받았을 때 호출되는 함수입니다. */
	UFUNCTION()
	void OnEventReceived(FGameplayEventData InPayload);

	/** 자식 클래스가 Event에 Task를 걸도록 열어둔 훅입니다. */
	virtual void RegisterAbilityEventTasks();
	/** 자식 클래스가 Event 수신 시 Task를 처리할 수 있도록 열어둔 훅입니다. */
	virtual void HandleAbilityEvent(const FGameplayEventData& InPayload);
	
	UFUNCTION()
	void ActiveFailed();
	
	void ResetCachedValues();

	UFUNCTION(BlueprintImplementableEvent, meta = (ToolTip = "Ability가 발동되어 실제로 동작이 트리거됐을 때 호출됩니다. Effect 적용 시점이 아닌, Ability의 동작이 기준입니다."))
	void OnEffectTriggered(const FGameplayTag& MontageEventTag, const TArray<AActor*>& TargetActors);

private:
	/** 발동 가능한 상태인지 검증합니다. */
	bool TryValidateAndCommitActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
	
	UFUNCTION(BlueprintPure, Category = "Effect")
	FText GetWeightDescription(const int32 Weight) const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> AbilityAnimMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ExcludeBaseStruct))
	TInstancedStruct<FEffectTargetTileSelector> EffectTargetTileSelector;

	TArray<FTargetSelectResult> CachedTargetSelectResults;
	TWeakObjectPtr<const ATile> CachedNoiseTargetTile;

#if WITH_EDITOR
public:
	virtual void PostInitProperties() override;
#endif
};
