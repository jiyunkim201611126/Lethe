// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCardAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/AbilitySystem/EffectApplier/GameplayEffectApplier.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
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

bool ULetheCardAbility::TryGetCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const
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

bool ULetheCardAbility::TryGetEffectsForSourcePreviewData(UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	if (!SourceASC)
	{
		return false;
	}
	
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (EffectApplier)
		{
			const TSubclassOf<UGameplayEffect>& SourcePreviewEffectClass = EffectApplier->GetSourcePreviewEffectClass();
			
			FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
			PreviewContextHandle.SetAbility(this);
			TArray<FGameplayEffectSpecHandle> SpecHandles;
			if (EffectApplier->TryMakeSpecHandlesForSourcePreview(SourceASC, PreviewContextHandle, SpecHandles))
			{
				TryGetGameplayEffectPreviewData(SourceASC, SourcePreviewEffectClass, SpecHandles, OutPreviewData);
			}
		}
	}
	return !OutPreviewData.IsEmpty();
}

bool ULetheCardAbility::TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, UAbilitySystemComponent* TargetASC, TMap<FGameplayAttribute, float>& OutPreviewDataForSource, TMap<FGameplayAttribute, float>& OutPreviewDataForTarget) const
{
	if (!SourceASC || !TargetASC)
	{
		return false;
	}
	
	for (const UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		// Ability 사용 시 효과는 대행자가 있으므로, EffectSpec을 만들도록 요청한 뒤 가져와 사용합니다.
		if (EffectApplier)
		{
			const TSubclassOf<UGameplayEffect>& EffectClass = EffectApplier->GetEffectClass();
			
			FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
			PreviewContextHandle.SetAbility(this);
			TArray<FGameplayEffectSpecHandle> SpecHandles;
			if (EffectApplier->TryMakeSpecHandles(SourceASC, PreviewContextHandle, SpecHandles, true))
			{
				TryGetGameplayEffectPreviewData(TargetASC, EffectClass, SpecHandles, OutPreviewDataForTarget);
			}
		}
	}

	// 반사 데미지, 흡혈 등 ExecCalc만으로는 구현 불가능한 규칙들을 Preview에도 적용하기 위해 아래 함수를 호출합니다.
	ULetheAbilitySystemLibrary::ResolveDamageRules(OutPreviewDataForSource, OutPreviewDataForTarget);

	return !OutPreviewDataForSource.IsEmpty() || !OutPreviewDataForTarget.IsEmpty();
}

bool ULetheCardAbility::TryGetGameplayEffectPreviewData(UAbilitySystemComponent* PreviewTargetASC, const TSubclassOf<UGameplayEffect>& EffectClass, TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const
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
				FGameplayEffectCustomExecutionParameters ExecutionParameters(*SpecHandle.Data.Get(), ExecDef.CalculationModifiers, PreviewTargetASC, ExecDef.PassedInTags, FPredictionKey());
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

	if (!TryValidateAndCommitActivation(Handle, ActorInfo, ActivationInfo, TriggerEventData))
	{
		ActiveFailed();
		return;
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
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

	UAbilityTask_WaitGameplayEvent* WaitEndAbilityEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		LetheGameplayTags.Event_Montage_EndAbility,
		nullptr,
		true,
		true);
	WaitEndAbilityEventTask->EventReceived.AddDynamic(this, &ThisClass::OnEventReceived);
	WaitEndAbilityEventTask->ReadyForActivation();

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

bool ULetheCardAbility::TryValidateAndCommitActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData || !TriggerEventData->Target)
	{
		return false;
	}
	
	// 애니메이션 재생 후 트리거를 통한 비동기 작업으로 Effect를 적용하기 때문에, 대상을 먼저 캐싱해둡니다.
	CachedTargetActor = const_cast<AActor*>(TriggerEventData->Target.Get());
	if (!CachedTargetActor.IsValid())
	{
		return false;
	}

	const AActor* SourceActor = GetAvatarActorFromActorInfo();
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const IAbilitySystemInterface* TargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(CachedTargetActor);
	const UAbilitySystemComponent* TargetASC = TargetAbilitySystemInterface ? TargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!SourceActor)
	{
		return false;
	}
	
	if (SourceActor->Implements<UPlayableCharacterInterface>())
	{
		// SourceActor가 플레이어 캐릭터인 경우 들어오는 분기입니다.
		if (!TargetASC || TargetASC->HasMatchingGameplayTag(LetheGameplayTags.State_Character_Dead))
		{
			return false;
		}
		
		// 적은 StateTreeTask에서 이미 검증된 Tile을 사용하기 때문에 플레이어 캐릭터에서만 FloorGap 로직을 수행합니다.
		const bool bCanUseByFloorGap = ULetheAbilitySystemLibrary::CanUseAbilityByActorAndFloorGap(this, SourceActor, CachedTargetActor.Get(), AbilityRange.FloorGap);
		if (bCanUseByFloorGap && CheckCost(Handle, ActorInfo))
		{
			// 플레이어 캐릭터인 경우에만 Cost 관련 로직을 수행합니다.
			CommitAbilityCost(Handle, ActorInfo, ActivationInfo);
			return true;
		}
	}
	else
	{
		// SourceActor가 적 캐릭터인 경우 들어오는 분기입니다.
		// 이 경우 Target Tile 위에 캐릭터가 없더라도, 애니메이션이나 나이아가라를 재생하기 위해 더미 액터를 올려두어 진행하기 때문에 true를 반환합니다.
		return true;
	}
	return false;
}

void ULetheCardAbility::OnEventReceived(FGameplayEventData Payload)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_ApplyEffect))
	{
		if (CachedTargetActor.IsValid() && CachedTargetActor->Implements<UAbilitySystemInterface>())
		{
			ApplyAllEffects(CachedTargetActor.Get());
			CachedTargetActor.Reset();
		}
	}
	else if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_EndAbility))
	{
		if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			LetheGameState->OnAbilityEnded(true);
			EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		}
	}
}

void ULetheCardAbility::ActiveFailed()
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnAbilityEnded(false);
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
	
	return FLetheTextManager::GetText(EStringTableType::CardDescription, RangeDescriptionKey, AbilityRange.Distance);
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
