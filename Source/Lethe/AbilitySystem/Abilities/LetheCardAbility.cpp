// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCardAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Lethe/AbilitySystem/EffectApplier/GameplayEffectApplier.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/LetheTextManager.h"

void ULetheCardAbility::ApplyAllEffects(AActor* TargetActor)
{
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier && TargetActor)
		{
			EffectApplier->ApplyEffect(this, TargetActor);
		}
	}
}

bool ULetheCardAbility::TryGetAbilityCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const
{
	if (CostGameplayEffectClass && SourceASC)
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

bool ULetheCardAbility::TryGetAbilityEffectsPreviewData(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TMap<FGameplayAttribute, float>& OutPreviewDataForSource, TMap<FGameplayAttribute, float>& OutPreviewDataForTarget) const
{
	if (!SourceASC || !TargetASC)
	{
		return false;
	}
	
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier)
		{
			FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
			PreviewContextHandle.SetAbility(this);

			if (AActor* TargetActor = TargetASC->GetAvatarActor())
			{
				TArray<TWeakObjectPtr<AActor>> TargetActors;
				TargetActors.Emplace(TargetActor);
				PreviewContextHandle.AddActors(TargetActors);
			}

			// EffectApplier에게서 Source와 Target의 Attribute에 영향을 주는 데이터를 추출하는 과정입니다.
			TArray<FPreviewEffectSpecContext> PreviewEffectSpecContexts;
			if (EffectApplier->TryBuildPreviewSpecContexts(SourceASC, TargetASC, PreviewContextHandle, PreviewEffectSpecContexts))
			{
				for (const FPreviewEffectSpecContext& PreviewEffectSpecContext : PreviewEffectSpecContexts)
				{
					UAbilitySystemComponent* PreviewASC;
					TMap<FGameplayAttribute, float>* OutPreviewData;
					
					if (PreviewEffectSpecContext.Target == EEffectPreviewTarget::Source)
					{
						PreviewASC = SourceASC;
						OutPreviewData = &OutPreviewDataForSource;
					}
					else
					{
						PreviewASC = TargetASC;
						OutPreviewData = &OutPreviewDataForTarget;
					}

					if (OutPreviewData)
					{
						TryGetGameplayEffectPreviewData(PreviewASC, PreviewEffectSpecContext.EffectClass, PreviewEffectSpecContext.SpecHandles, *OutPreviewData);
					}
				}
			}
		}
	}
	return !OutPreviewDataForSource.IsEmpty() || !OutPreviewDataForTarget.IsEmpty();
}

bool ULetheCardAbility::TryGetGameplayEffectPreviewData(UAbilitySystemComponent* TargetASC, const TSubclassOf<UGameplayEffect>& EffectClass, const TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	// GameplayEffect가 적용됐을 때 어떤 변화값이 있는지 가져와서 OutData에 채워줍니다.
	if (const UGameplayEffect* GameplayEffectCDO = EffectClass.GetDefaultObject())
	{
		for (const FGameplayEffectSpecHandle& SpecHandle : SpecHandles)
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

FGameplayEffectContextHandle ULetheCardAbility::GetContextHandle(const TSubclassOf<UGameplayEffectApplier>& ApplierClass) const
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

void ULetheCardAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!TriggerEventData || !TriggerEventData->Target)
	{
		ActiveFailed();
		return;
	}
	
	// 애니메이션 재생 후 트리거를 통한 비동기 작업으로 Effect를 적용하기 때문에, 대상을 먼저 캐싱해둡니다.
	CachedTargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!CachedTargetActor.IsValid())
	{
		ActiveFailed();
		return;
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const IAbilitySystemInterface* TargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(CachedTargetActor);
	const UAbilitySystemComponent* TargetASC = TargetAbilitySystemInterface ? TargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!TargetASC || TargetASC->HasMatchingGameplayTag(LetheGameplayTags.State_Character_Dead))
	{
		ActiveFailed();
		return;
	}

	if (CheckCost(Handle, ActorInfo))
	{
		CommitAbilityCost(Handle, ActorInfo, ActivationInfo);
		
		// 어떤 CardAbility를 사용하든, 한 번 사용하고 나면 해당 턴에서 더이상 움직일 수 없습니다.
		ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(LetheGameplayTags.State_Character_MoveConsumed);
		
		UAbilityTask_WaitGameplayEvent* WaitApplyEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			LetheGameplayTags.Event_Montage_ApplyEffect,
			nullptr,
			true,
			true);
		WaitApplyEffectEventTask->EventReceived.AddDynamic(this, &ThisClass::OnEventReceived);
		WaitApplyEffectEventTask->ReadyForActivation();
	
		UAbilityTask_WaitGameplayEvent* WaitEndUseCardEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			LetheGameplayTags.Event_Montage_EndUseCard,
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
	else
	{
		ActiveFailed();
	}
}

void ULetheCardAbility::OnEventReceived(FGameplayEventData Payload)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_ApplyEffect))
	{
		if (CachedTargetActor.IsValid())
		{
			ApplyAllEffects(CachedTargetActor.Get());
			CachedTargetActor.Reset();
		}
	}
	else if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_EndUseCard))
	{
		if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
		{
			LethePlayerController->OnAbilityEnded(true);
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		}
	}
}

void ULetheCardAbility::ActiveFailed()
{
	// GameplayEffect를 적용할 대상이 이미 사망한 경우 로직을 중단합니다.
	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		LethePlayerController->OnAbilityEnded(false);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
	}
}

void ULetheCardAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	// 프로젝트 특성상 한 번 발동된 Ability가 Cancel될 수는 없으나 일단 구현해두었습니다.
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier)
		{
			EffectApplier->CancelAbility();
		}
	}
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

void ULetheCardAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
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

FText ULetheCardAbility::GetRangeDescription() const
{
	FString RangeDescriptionKey;
	switch (AbilityRange.BFSType)
	{
	case EBFSType::Connection:
		RangeDescriptionKey = TEXT("Range.Connection");
		break;
	case EBFSType::Through:
		RangeDescriptionKey = TEXT("Range.Through");
		break;
	}
	
	return FLetheTextManager::GetText(EStringTableType::CardDescription, RangeDescriptionKey, AbilityRange.Depth);
}

void ULetheCardAbility::PostInitProperties()
{
	Super::PostInitProperties();

	// 어떤 CardAbility든 자신의 턴에만 사용할 수 있습니다.
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		
		ActivationRequiredTags.AddTag(LetheGameplayTags.State_Character_CanAct);
	}
}
