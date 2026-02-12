// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

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
	return !OutPreviewData.IsEmpty();
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
		CommitAbilityCost(Handle, ActorInfo, ActivationInfo);
		
		if (TriggerEventData && TriggerEventData->Target)
		{
			// 애니메이션 재생을 통한 비동기 작업으로 Effect를 적용하기 때문에, 대상을 먼저 캐싱해둡니다.
			CachedTargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
			
			const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
			UAbilityTask_WaitGameplayEvent* WaitApplyEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				LetheGameplayTags.MontageEvent_ApplyEffect,
				nullptr,
				true,
				true);
			WaitApplyEffectEventTask->EventReceived.AddDynamic(this, &ThisClass::OnEventReceived);
			WaitApplyEffectEventTask->ReadyForActivation();
		
			UAbilityTask_WaitGameplayEvent* WaitEndUseCardEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
				this,
				LetheGameplayTags.MontageEvent_EndUseCard,
				nullptr,
				true,
				true);
			WaitEndUseCardEventTask->EventReceived.AddDynamic(this, &ThisClass::OnEventReceived);
			WaitEndUseCardEventTask->ReadyForActivation();

			UAbilityTask_PlayMontageAndWait* PlayMontageAndWaitTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				AbilityAnimMontage,
				1.f,
				NAME_None,
				false,
				1.f);
			PlayMontageAndWaitTask->ReadyForActivation();
		}
	}
}

void ULetheGameplayAbility::OnEventReceived(FGameplayEventData Payload)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.MontageEvent_ApplyEffect))
	{
		if (CachedTargetActor.IsValid())
		{
			ApplyAllEffects(CachedTargetActor.Get());
			CachedTargetActor.Reset();

			// Effect 적용 타이밍에 카드 사용 성공 이벤트를 발생시킵니다.
			// Ability 발동 타이밍에 발생시킬 경우 타이밍 이슈로 CardPanel이 정상적으로 동작하지 못 하기 때문에, 살짝 늦은 타이밍에 보내야 하는데 이 위치가 제격입니다.
			GetAbilitySystemComponentFromActorInfo()->HandleGameplayEvent(LetheGameplayTags.Event_UseCardSuccess, new FGameplayEventData());
		}
	}
	else if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.MontageEvent_EndUseCard))
	{
		if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			LethePlayerController->OnUseCardEnded();
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
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
void ULetheGameplayAbility::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		ActivationBlockedTags.AddTag(FLetheGameplayTags::Get().CharacterState_Dead);
	}
}
#endif
