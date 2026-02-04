// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

#include "GameplayEffectExecutionCalculation.h"

void ULetheGameplayAbility::ApplyAllEffects(AActor* TargetActor)
{
	for (auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			FGameplayEffectApplier& EffectApplier = InstancedApplier.GetMutable();
			if (TargetActor)
			{
				EffectApplier.ApplyEffect(this, TargetActor);
			}
		}
	}
}

FText ULetheGameplayAbility::GetCardDescription(const int32 InLevel) const
{
	TArray<FText> ResultTexts;
	for (const auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			const FGameplayEffectApplier& EffectApplier = InstancedApplier.Get();
			ResultTexts.Emplace(EffectApplier.GetDescriptionText(InLevel));
		}
	}

	return FText::Join(FText::FromString(TEXT(" ")), ResultTexts);
}

FAbilityRange ULetheGameplayAbility::GetAbilityRange() const
{
	return AbilityRange;
}

bool ULetheGameplayAbility::TryGetAbilityCostEffectPreviewData(TMap<FGameplayAttribute, float>& OutCostPreviewData) const
{
	if (CostGameplayEffectClass)
	{
		const UGameplayEffect* CostGE = CostGameplayEffectClass->GetDefaultObject<UGameplayEffect>();
		return TryGetGameplayEffectPreviewData(nullptr, CostGE, OutCostPreviewData);
	}
	return false;
}

bool ULetheGameplayAbility::TryGetAllEffectsPreviewData(UAbilitySystemComponent* TargetASC, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	for (const auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			const FGameplayEffectApplier& EffectApplier = InstancedApplier.Get();
			const TSubclassOf<UGameplayEffect>& EffectClass = EffectApplier.GetEffectClass();
			if (const UGameplayEffect* GameplayEffectCDO = EffectClass.GetDefaultObject())
			{
				TryGetGameplayEffectPreviewData(TargetASC, GameplayEffectCDO, OutPreviewData);
			}
		}
	}
	return !OutPreviewData.IsEmpty();
}

bool ULetheGameplayAbility::TryGetGameplayEffectPreviewData(UAbilitySystemComponent* TargetASC, const UGameplayEffect* GameplayEffectCDO, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	// GameplayEffect가 적용됐을 때 어떤 변화값이 있는지 가져와서 OutData에 채워줍니다.
	if (GameplayEffectCDO)
	{
		FGameplayEffectSpec CostSpec(GameplayEffectCDO, FGameplayEffectContextHandle(), 1.f);

		for (const FGameplayModifierInfo& Modifier : GameplayEffectCDO->Modifiers)
		{
			float Magnitude = 0.f;
			if (Modifier.ModifierMagnitude.AttemptCalculateMagnitude(CostSpec, Magnitude))
			{
				OutPreviewData.FindOrAdd(Modifier.Attribute) += Magnitude;
			}
		}

		// 단순 Modifier 외에도 ExecCalc에 해당하는 변화값도 뽑아옵니다.
		for (const FGameplayEffectExecutionDefinition& ExecDef : GameplayEffectCDO->Executions)
		{
			if (!ExecDef.CalculationClass)
			{
				continue;
			}
			
			const UGameplayEffectExecutionCalculation* ExecCalcCDO = ExecDef.CalculationClass->GetDefaultObject<UGameplayEffectExecutionCalculation>();
			if (!ExecCalcCDO)
			{
				continue;
			}

			// ExecCalc의 계산을 실제로 한 번 돌립니다.
			FGameplayEffectCustomExecutionParameters ExecutionParameters(CostSpec, ExecDef.CalculationModifiers, TargetASC, ExecDef.PassedInTags, FPredictionKey());
			FGameplayEffectCustomExecutionOutput ExecutionOutput;
			ExecCalcCDO->Execute(ExecutionParameters, ExecutionOutput);

			TArray<FGameplayModifierEvaluatedData> OutModifiers;
			ExecutionOutput.GetOutputModifiers(OutModifiers);

			// ExecCalc 계산을 통해 나온 Modifier를 OutData에 채워줍니다.
			for (const FGameplayModifierEvaluatedData& Modifier : OutModifiers)
			{
				OutPreviewData.FindOrAdd(Modifier.Attribute) += Modifier.Magnitude;
			}
		}
		return true;
	}
	return false;
}

FGameplayEffectContextHandle ULetheGameplayAbility::GetContextHandle(const int32 ApplierIndex) const
{
	if (EffectAppliers.IsValidIndex(ApplierIndex))
	{
		const auto& InstancedApplier = EffectAppliers[ApplierIndex];
		if (InstancedApplier.IsValid())
		{
			const FGameplayEffectApplier& EffectApplier = InstancedApplier.Get();
			return EffectApplier.GetEffectContextHandle();
		}
	}
	return FGameplayEffectContextHandle();
}

void ULetheGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (CheckCost(Handle, ActorInfo))
	{
		if (TriggerEventData && TriggerEventData->Target)
		{
			CommitAbilityCost(Handle, ActorInfo, ActivationInfo);
			ApplyAllEffects(const_cast<AActor*>(TriggerEventData->Target.Get()));
		}
	}
}

void ULetheGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	for (auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			FGameplayEffectApplier& EffectApplier = InstancedApplier.GetMutable();
			EffectApplier.CancelAbility();
		}
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void ULetheGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	for (auto& InstancedApplier : EffectAppliers)
	{
		if (InstancedApplier.IsValid())
		{
			FGameplayEffectApplier& EffectApplier = InstancedApplier.GetMutable();
			EffectApplier.EndAbility();
		}
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


#if WITH_EDITOR
#include "Lethe/Manager/LetheGameplayTags.h"

void ULetheGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().CharacterState_Dead);
	}
}
#endif
