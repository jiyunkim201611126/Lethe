// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"

void ULetheGameplayAbility::ApplyAllEffects(AActor* TargetActor)
{
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier && TargetActor)
		{
			EffectApplier->ApplyEffect(this, TargetActor);
		}
	}
}

FText ULetheGameplayAbility::GetCardDescription(const int32 InLevel) const
{
	TArray<FText> ResultTexts;
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier)
		{
			ResultTexts.Emplace(EffectApplier->GetDescriptionText(InLevel));
		}
	}

	return FText::Join(FText::FromString(TEXT(" ")), ResultTexts);
}

FAbilityRange ULetheGameplayAbility::GetAbilityRange() const
{
	return AbilityRange;
}

bool ULetheGameplayAbility::TryGetAbilityCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const
{
	if (CostGameplayEffectClass)
	{
		// Ability Cost는 Ability가 소유자이므로, EffectSpec을 직접 만들어서 Preview Data를 추출합니다.
		FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
		PreviewContextHandle.SetAbility(this);
		FGameplayEffectSpecHandle CostEffectSpecHandle = SourceASC->MakeOutgoingSpec(CostGameplayEffectClass, 1.f, PreviewContextHandle);
		
		TArray<FGameplayEffectSpecHandle> CostEffectSpecHandleArray;
		CostEffectSpecHandleArray.Emplace(CostEffectSpecHandle);
		
		return TryGetGameplayEffectPreviewData(nullptr, CostGameplayEffectClass, CostEffectSpecHandleArray, OutCostPreviewData);
	}
	return false;
}

bool ULetheGameplayAbility::TryGetAbilityEffectsPreviewData(const UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		// Ability 사용 시 효과는 대행자가 있으므로, EffectSpec을 만들도록 요청한 뒤 가져와 사용합니다.
		if (EffectApplier)
		{
			const TSubclassOf<UGameplayEffect>& EffectClass = EffectApplier->GetEffectClass();
			
			FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
			PreviewContextHandle.SetAbility(this);
			TArray<FGameplayEffectSpecHandle> SpecHandles;
			if (EffectApplier->TryMakeSpecHandles(SourceASC, this, PreviewContextHandle, SpecHandles))
			{
				TryGetGameplayEffectPreviewData(TargetASC, EffectClass, SpecHandles, OutPreviewData);
			}
		}
	}
	return false;
}

bool ULetheGameplayAbility::TryGetGameplayEffectPreviewData(UAbilitySystemComponent* TargetASC, const TSubclassOf<UGameplayEffect>& EffectClass, TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	// GameplayEffect가 적용됐을 때 어떤 변화값이 있는지 가져와서 OutData에 채워줍니다.
	if (const UGameplayEffect* GameplayEffectCDO = EffectClass.GetDefaultObject())
	{
		for (auto& SpecHandle : SpecHandles)
		{
			if (!SpecHandle.Data.IsValid())
			{
				continue;
			}
			
			for (const FGameplayModifierInfo& Modifier : GameplayEffectCDO->Modifiers)
			{
				float Magnitude = 0.f;
				if (Modifier.ModifierMagnitude.AttemptCalculateMagnitude(*SpecHandle.Data.Get(), Magnitude))
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
				FGameplayEffectCustomExecutionParameters ExecutionParameters(*SpecHandle.Data.Get(), ExecDef.CalculationModifiers, TargetASC, ExecDef.PassedInTags, FPredictionKey());
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
		}
		return true;
	}
	return false;
}

FGameplayEffectContextHandle ULetheGameplayAbility::GetContextHandle(const TSubclassOf<UGameplayEffectApplier>& ApplierClass) const
{
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier && EffectApplier->GetClass() == ApplierClass)
		{
			return EffectApplier->GetEffectContextHandle();
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
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier)
		{
			EffectApplier->CancelAbility();
		}
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void ULetheGameplayAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier)
		{
			EffectApplier->EndAbility();
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
