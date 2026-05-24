// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCardAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/EffectApplier/GameplayEffectApplier.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/LetheTextManager.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ULetheCardAbility::ULetheCardAbility()
{
	FEffectApplyPolicy& EffectApplyPolicy = EffectApplyPolicies.Emplace_GetRef();
	EffectApplyPolicy.MontageEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.1"));
}

void ULetheCardAbility::GetTargetTiles(const AActor* AvatarActor, ATile* CenterTile, TArray<TWeakObjectPtr<ATile>>& OutTargetTiles) const
{
	if (!EffectTargetSelector)
	{
		OutTargetTiles.Add(CenterTile);
		return;
	}

	EffectTargetSelector->Select(AvatarActor, CenterTile, OutTargetTiles);
}

bool ULetheCardAbility::TryGetCostEffectPreviewData(const UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const
{
	if (CostGameplayEffectClass && SourceASC)
	{
		// Ability Cost는 Ability가 소유자이므로, EffectSpec을 직접 만들어서 Preview Data를 추출합니다.
		FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
		PreviewContextHandle.SetAbility(this);
		const FGameplayEffectSpecHandle CostEffectSpecHandle = SourceASC->MakeOutgoingSpec(CostGameplayEffectClass, 1.f, PreviewContextHandle);
		
		TArray<FGameplayEffectSpecHandle> CostEffectSpecHandleArray;
		CostEffectSpecHandleArray.Add(CostEffectSpecHandle);
		
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
	
	for (const FEffectApplyPolicy& EffectApplyPolicy : EffectApplyPolicies)
	{
		TArray<UGameplayEffectApplier*> PolicyEffectAppliers;
		GetEffectAppliersByPolicy(EffectApplyPolicy, PolicyEffectAppliers);
		
		for (const UGameplayEffectApplier* EffectApplier : PolicyEffectAppliers)
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

bool ULetheCardAbility::TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<AActor*>& TargetActors, FGameplayEffectPreviewData& OutPreviewData) const
{
	if (!SourceASC || TargetActors.IsEmpty())
	{
		return false;
	}
	
	for (const FEffectApplyPolicy& EffectApplyPolicy : EffectApplyPolicies)
	{
		TArray<AActor*> PolicyTargetActors;
		GetTargetActorsByPolicy(EffectApplyPolicy, TargetActors, PolicyTargetActors);

		TArray<UGameplayEffectApplier*> PolicyEffectAppliers;
		GetEffectAppliersByPolicy(EffectApplyPolicy, PolicyEffectAppliers);
		
		for (AActor* TargetActor : PolicyTargetActors)
		{
			const IAbilitySystemInterface* TargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
			UAbilitySystemComponent* TargetASC = TargetAbilitySystemInterface ? TargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
			if (!TargetASC)
			{
				continue;
			}
			
			TMap<FGameplayAttribute, float>& OutPreviewDataForTarget = OutPreviewData.TargetPreviewData.FindOrAdd(TargetASC);
			for (const UGameplayEffectApplier* EffectApplier : PolicyEffectAppliers)
			{
				// EffectApplier에게 EffectSpec을 만들도록 요청한 뒤 가져와 사용합니다.
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
	}
	
	// 반사 데미지, 흡혈 등 ExecCalc만으로는 구현 불가능한 규칙들을 Preview에도 적용하기 위해 아래 로직을 수행합니다.
	for (auto& TargetPreviewData : OutPreviewData.TargetPreviewData)
	{
		const UAbilitySystemComponent* TargetASC = TargetPreviewData.Key;
		TMap<FGameplayAttribute, float>& OutPreviewDataForTarget = TargetPreviewData.Value;
		if (TargetASC)
		{
			if (const float* IncomingDamage = OutPreviewDataForTarget.Find(ULetheAttributeSet::GetIncomingDamageAttribute()))
			{
				ULetheAbilitySystemLibrary::ResolveDamageRules(SourceASC, TargetASC, *IncomingDamage, OutPreviewData.SourcePreviewData, OutPreviewDataForTarget);
			}
		}
	}

	return !OutPreviewData.IsEmpty();
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

	// 갖고 있는 모든 EffectApplyPolicy의 MontageEventTag로 WaitGameplayEvent Task를 생성합니다.
	for (const FEffectApplyPolicy& ApplyPolicy : EffectApplyPolicies)
	{
		UAbilityTask_WaitGameplayEvent* WaitApplyEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			ApplyPolicy.MontageEventTag,
			nullptr,
			true,
			true);
		WaitApplyEffectEventTask->EventReceived.AddDynamic(this, &ThisClass::OnEventReceived);
		WaitApplyEffectEventTask->ReadyForActivation();
	}

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

	// 플레이어 캐릭터인 경우 소음 발생 로직을 시작합니다.
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			const ATile* StandingTile = TileManagerSubsystem->GetTileUnderActor(AvatarActor);
			const ATile* TargetTile = TileManagerSubsystem->GetTileUnderActor(CachedCenterTargetTile.Get());
			ActivateNoise(StandingTile, TargetTile);
		}
	}
}

bool ULetheCardAbility::TryValidateAndCommitActivation(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!TriggerEventData)
	{
		return false;
	}
	
	// 애니메이션 재생 후 트리거를 통한 비동기 작업으로 Effect를 적용하기 때문에, 대상을 먼저 캐싱해둡니다.
	const FGameplayAbilityTargetDataHandle& TargetDataHandle = TriggerEventData->TargetData;
	const FGameplayAbilityTargetData* TargetData = TargetDataHandle.Get(0);
	if (!TargetData)
	{
		return false;
	}

	CachedTargetActors = TargetData->GetActors();
	CachedCenterTargetTile = Cast<const ATile>(TriggerEventData->OptionalObject);

	if (CachedTargetActors.IsEmpty() || !CachedCenterTargetTile.IsValid())
	{
		ResetCachedValues();
		return false;
	}

	const AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceActor)
	{
		ResetCachedValues();
		return false;
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	TArray<const UAbilitySystemComponent*> TargetASCs;
	for (const auto& TargetActor : CachedTargetActors)
	{
		if (TargetActor.IsValid())
		{
			if (const IAbilitySystemInterface* TargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor))
			{
				TargetASCs.Add(TargetAbilitySystemInterface->GetAbilitySystemComponent());
			}
		}
	}
	
	if (SourceActor->Implements<UPlayerCharacterInterface>())
	{
		// SourceActor가 플레이어 캐릭터인 경우 들어오는 분기입니다.
		// 이미 사망한 캐릭터는 Target에서 제외합니다.
		TargetASCs.RemoveAll([LetheGameplayTags](const UAbilitySystemComponent* ASC)
		{
			return ASC->HasMatchingGameplayTag(LetheGameplayTags.State_Character_Dead);
		});

		// 적은 StateTreeTask에서 이미 검증된 Tile을 사용하기 때문에 플레이어 캐릭터에서만 FloorGap 로직을 수행합니다.
		TargetASCs.RemoveAll([this, SourceActor](const UAbilitySystemComponent* ASC)
		{
			return !ULetheAbilitySystemLibrary::CanUseAbilityByActorAndFloorGap(SourceActor, ASC->GetOwnerActor(), AbilityRange.FloorGap);
		});

		// 위 두 조건을 거친 후, 공격 가능한 Target이 남아있지 않다면 false를 반환합니다.
		if (TargetASCs.IsEmpty())
		{
			ResetCachedValues();
			return false;
		}
		
		// 플레이어 캐릭터인 경우에만 Cost 관련 로직을 수행합니다.
		CommitAbilityCost(Handle, ActorInfo, ActivationInfo);
		return true;
	}
	
	// SourceActor가 적 캐릭터인 경우 이곳으로 내려옵니다.
	// 이 경우 Target Tile 위에 캐릭터가 없더라도, 애니메이션이나 나이아가라를 재생하기 위해 더미 액터를 올려두어 진행하기 때문에 true를 반환합니다.
	return true;
}

void ULetheCardAbility::OnEventReceived(FGameplayEventData Payload)
{
	for (const FEffectApplyPolicy& EffectApplyPolicy : EffectApplyPolicies)
	{
		if (Payload.EventTag.MatchesTagExact(EffectApplyPolicy.MontageEventTag))
		{
			// 수신한 이벤트 태그와 EffectApplyPolicy의 이벤트 태그가 일치하는 경우 들어오는 분기입니다.
			TArray<AActor*> TargetActors;
			TargetActors.Reserve(CachedTargetActors.Num());
			for (const auto& CachedTargetActor : CachedTargetActors)
			{
				if (CachedTargetActor.IsValid())
				{
					TargetActors.Add(CachedTargetActor.Get());
				}
			}

			TArray<AActor*> OutTargetActors;
			GetTargetActorsByPolicy(EffectApplyPolicy, TargetActors, OutTargetActors);

			if (!OutTargetActors.IsEmpty())
			{
				for (AActor* TargetActor : OutTargetActors)
				{
					ApplyEffectsByPolicy(EffectApplyPolicy, TargetActor);
				}
				OnApplyEffect(EffectApplyPolicy.MontageEventTag, OutTargetActors);
			}
			return;
		}
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (Payload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_EndAbility))
	{
		ResetCachedValues();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
	}
}

void ULetheCardAbility::GetTargetActorsByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, const TArray<AActor*>& SourceTargetActors, TArray<AActor*>& OutTargetActors) const
{
	if (EffectApplyPolicy.TargetActorIndices.Contains(FEffectApplyPolicy::AllIndices))
	{
		// 모든 TargetActor에게 Effect를 적용해야 하는 경우 들어오는 분기입니다.
		for (AActor* TargetActor : SourceTargetActors)
		{
			if (TargetActor)
			{
				OutTargetActors.AddUnique(TargetActor);
			}
		}
		return;
	}
	
	// TargetActorIndex번째 TargetActor에게 Effect를 적용하는 정책인 경우, SourceTargetActors에서 가져와 Out배열에 추가합니다.
	for (const int32 TargetActorIndex : EffectApplyPolicy.TargetActorIndices)
	{
		if (SourceTargetActors.IsValidIndex(TargetActorIndex) && SourceTargetActors[TargetActorIndex])
		{
			OutTargetActors.AddUnique(SourceTargetActors[TargetActorIndex]);
		}
	}
}

void ULetheCardAbility::ApplyEffectsByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, AActor* TargetActor)
{
	if (!TargetActor)
	{
		return;
	}

	TArray<UGameplayEffectApplier*> OutEffectAppliers;
	GetEffectAppliersByPolicy(EffectApplyPolicy, OutEffectAppliers);
	
	for (UGameplayEffectApplier* EffectApplier : OutEffectAppliers)
	{
		EffectApplier->ApplyEffect(this, TargetActor);
	}
}

void ULetheCardAbility::GetEffectAppliersByPolicy(const FEffectApplyPolicy& EffectApplyPolicy, TArray<UGameplayEffectApplier*>& OutEffectAppliers) const
{
	for (UGameplayEffectApplier* EffectApplier : EffectAppliers)
	{
		if (!EffectApplier)
		{
			continue;
		}

		if (EffectApplyPolicy.EffectApplierTags.HasTagExact(EffectApplier->GetEffectApplierTag()))
		{
			OutEffectAppliers.AddUnique(EffectApplier);
		}
		else
		{
			LETHE_LOG(LogAbility, Warning, "소유하고 있지 않은 EffectApplier의 GameplayTag를 할당받은 EffectApplyPolicy가 존재합니다. Ability: %s", *GetName());
		}
	}
}

void ULetheCardAbility::ActiveFailed()
{
	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnAbilityActivationFailed();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
	}
}

void ULetheCardAbility::ResetCachedValues()
{
	CachedTargetActors.Empty();
	CachedCenterTargetTile.Reset();
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
	
	ResetCachedValues();
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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
	
	ResetCachedValues();
	
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);
}

bool ULetheCardAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}

	// 플레이어 캐릭터인 경우에만 Cost 관련 로직을 수행합니다.
	if (ActorInfo->AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags);
	}
	return true;
}

bool ULetheCardAbility::CommitAbilityCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, FGameplayTagContainer* OptionalRelevantTags)
{
	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		return false;
	}
	
	// 플레이어 캐릭터인 경우에만 Cost 관련 로직을 수행합니다.
	if (ActorInfo->AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		return Super::CommitAbilityCost(Handle, ActorInfo, ActivationInfo, OptionalRelevantTags);
	}
	return true;
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

FText ULetheCardAbility::GetWeightDescription(const int32 Weight) const
{
	const FString WeightDescriptionKey = TEXT("Weight");
	return FLetheTextManager::GetText(EStringTableType::CardDescription, WeightDescriptionKey, Weight);
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
