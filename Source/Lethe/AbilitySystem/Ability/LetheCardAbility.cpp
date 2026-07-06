// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCardAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/EffectDelivery/EffectDelivery_Immediately.h"
#include "Lethe/AbilitySystem/EffectSpecBuilder/GameplayEffectSpecBuilder.h"
#include "Lethe/AbilitySystem/EffectDelivery/GameplayEffectDelivery.h"
#include "Lethe/AbilitySystem/EffectTargetTileSelector/EffectTargetTileSelector.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/LetheTextManager.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ULetheCardAbility::ULetheCardAbility()
{
	FEffectTargetMappingPolicy& EffectTargetMappingPolicy = EffectTargetMappingPolicies.Emplace_GetRef();
	EffectTargetMappingPolicy.MontageEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.1"));

	EffectDelivery.InitializeAs<FEffectDelivery_Immediately>();
}

void ULetheCardAbility::GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles) const
{
	if (EffectTargetTileSelector)
	{
		EffectTargetTileSelector->GetCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles, OutTargetCandidateTiles);
	}
}

void ULetheCardAbility::GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	if (EffectTargetTileSelector)
	{
		EffectTargetTileSelector->GetTargetTiles(AvatarActor, PlayerController, OutTiles);
	}
}

int32 ULetheCardAbility::GetEffectSpecBuilderValueForDescription(const FGameplayTag& EffectSpecBuilderTag, const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	for (const auto& EffectSpecBuilder : EffectSpecBuilders)
	{
		if (const FGameplayEffectSpecBuilder* EffectSpecBuilderPtr = EffectSpecBuilder.GetPtr())
		{
			if (EffectSpecBuilderPtr->GetEffectSpecBuilderTag().MatchesTagExact(EffectSpecBuilderTag))
			{
				return EffectSpecBuilderPtr->GetValueForDescription(OwnerASC, InLevel);
			}
		}
	}
	return 0;
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

		return TryGetGameplayEffectPreviewData(nullptr, CostEffectSpecHandleArray, OutCostPreviewData);
	}
	return false;
}

bool ULetheCardAbility::TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<AActor*>& TargetActors, FGameplayEffectPreviewData& OutPreviewData) const
{
	if (!SourceASC || TargetActors.IsEmpty())
	{
		return false;
	}

	for (const FEffectTargetMappingPolicy& EffectTargetMappingPolicy : EffectTargetMappingPolicies)
	{
		FEffectTargetMappingResolveResult ResolveResult;
		ResolveEffectTargetMappingPolicy(EffectTargetMappingPolicy, SourceASC, TargetActors, ResolveResult);

		TryGetGameplayEffectPreviewData(SourceASC, ResolveResult.SourceSpecHandles, OutPreviewData.SourcePreviewData);

		for (const auto& Pair : ResolveResult.TargetSpecHandlesByActor)
		{
			if (!Pair.Key)
			{
				continue;
			}

			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pair.Key);
			if (!TargetASC)
			{
				continue;
			}

			TMap<FGameplayAttribute, float>& OutPreviewDataForTarget = OutPreviewData.TargetPreviewData.FindOrAdd(TargetASC);

			TryGetGameplayEffectPreviewData(TargetASC, Pair.Value, OutPreviewDataForTarget);
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

bool ULetheCardAbility::TryGetGameplayEffectPreviewData(UAbilitySystemComponent* PreviewTargetASC, const TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	// GameplayEffect가 적용됐을 때 어떤 변화값이 있는지 가져와서 OutData에 채워줍니다.
	for (auto& SpecHandle : SpecHandles)
	{
		const FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
		if (!Spec || !Spec->Def)
		{
			continue;
		}

		const UGameplayEffect* GameplayEffectCDO = Spec->Def;

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
	return !OutPreviewData.IsEmpty();
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

	// 갖고 있는 모든 EffectTargetMappingPolicy의 MontageEventTag로 WaitGameplayEvent Task를 생성합니다.
	for (const FEffectTargetMappingPolicy& EffectTargetMappingPolicy : EffectTargetMappingPolicies)
	{
		UAbilityTask_WaitGameplayEvent* WaitApplyEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			EffectTargetMappingPolicy.MontageEventTag,
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
			if (const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor.Get()))
			{
				TargetASCs.Add(TargetASC);
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

void ULetheCardAbility::OnEventReceived(FGameplayEventData InPayload)
{
	TArray<AActor*> TargetActors;
	TargetActors.Reserve(CachedTargetActors.Num());
	for (const auto& CachedTargetActor : CachedTargetActors)
	{
		// EffectTargetMappingPolicies에서 TargetActors의 인덱스를 기반으로 로직을 수행하기 때문에, nullptr도 추가해야 합니다.
		TargetActors.Add(CachedTargetActor.Get());
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	for (const FEffectTargetMappingPolicy& EffectTargetMappingPolicy : EffectTargetMappingPolicies)
	{
		if (InPayload.EventTag.MatchesTagExact(EffectTargetMappingPolicy.MontageEventTag))
		{
			// 수신한 이벤트 태그와 EffectTargetMappingPolicy의 이벤트 태그가 일치하는 경우 들어오는 분기입니다.
			FEffectTargetMappingResolveResult ResolveResult;
			ResolveEffectTargetMappingPolicy(EffectTargetMappingPolicy, SourceASC, TargetActors, ResolveResult);

			for (const FGameplayEffectSpecHandle& SourceSpecHandle : ResolveResult.SourceSpecHandles)
			{
				if (SourceSpecHandle.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToSelf(*SourceSpecHandle.Data.Get());
				}
			}

			TArray<AActor*> EffectedTargetActors;
			for (const auto& Pair : ResolveResult.TargetSpecHandlesByActor)
			{
				if (Pair.Key)
				{
					EffectedTargetActors.Add(Pair.Key);
					StartDeliveryEffects(Pair.Key, Pair.Value);
				}
			}

			if (!EffectedTargetActors.IsEmpty())
			{
				OnEffectTriggered(EffectTargetMappingPolicy.MontageEventTag, EffectedTargetActors);
			}
			return;
		}
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (InPayload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_EndAbility))
	{
		ResetCachedValues();
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
	}
}

void ULetheCardAbility::ResolveEffectTargetMappingPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, UAbilitySystemComponent* SourceASC, const TArray<AActor*>& CandidateTargetActors, FEffectTargetMappingResolveResult& OutResult) const
{
	OutResult.SourceSpecHandles.Reset();
	OutResult.TargetSpecHandlesByActor.Reset();

	if (!SourceASC)
	{
		return;
	}

	// Policy에 해당하는 TargetActors를 가져옵니다.
	TArray<AActor*> OutTargetActors;
	GetTargetActorsByPolicy(EffectTargetMappingPolicy, CandidateTargetActors, OutTargetActors);

	// Policy에 해당하는 EffectSpecBuilder를 가져옵니다.
	TArray<const FGameplayEffectSpecBuilder*> OutEffectSpecBuilders;
	GetEffectSpecBuildersByPolicy(EffectTargetMappingPolicy, OutEffectSpecBuilders);

	if (OutTargetActors.IsEmpty() || OutEffectSpecBuilders.IsEmpty())
	{
		return;
	}

	// Source와 Target에게 적용할 모든 Spec을 Out 인자에 추가합니다.
	for (const FGameplayEffectSpecBuilder* EffectSpecBuilder : OutEffectSpecBuilders)
	{
		FGameplayEffectContextHandle SourceEffectContextHandle = SourceASC->MakeEffectContext();
		SourceEffectContextHandle.SetAbility(this);

		TArray<FGameplayEffectSpecHandle> SourceSpecHandles;
		if (EffectSpecBuilder->TryBuildSourceEffectSpecs(SourceASC, SourceEffectContextHandle, SourceSpecHandles))
		{
			OutResult.SourceSpecHandles.Append(SourceSpecHandles);
		}

		for (AActor* TargetActor : OutTargetActors)
		{
			// TargetActor별로 Context를 세팅합니다.
			FGameplayEffectContextHandle EffectContextHandle = SourceASC->MakeEffectContext();
			EffectContextHandle.SetAbility(this);
			TArray<TWeakObjectPtr<AActor>> TargetActorArray;
			TargetActorArray.Add(TargetActor);
			EffectContextHandle.AddActors(TargetActorArray);

			TArray<FGameplayEffectSpecHandle> TargetSpecHandlesForActor;
			if (EffectSpecBuilder->TryBuildTargetEffectSpecs(SourceASC, EffectContextHandle, TargetSpecHandlesForActor))
			{
				if (!TargetSpecHandlesForActor.IsEmpty())
				{
					TArray<FGameplayEffectSpecHandle>& TargetSpecHandles = OutResult.TargetSpecHandlesByActor.FindOrAdd(TargetActor);
					TargetSpecHandles.Append(TargetSpecHandlesForActor);
				}
			}
		}
	}
}

void ULetheCardAbility::GetTargetActorsByPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, const TArray<AActor*>& CandidateTargetActors, TArray<AActor*>& OutTargetActors) const
{
	if (EffectTargetMappingPolicy.TargetActorIndices.Contains(FEffectTargetMappingPolicy::AllIndices))
	{
		// 모든 TargetActor에게 Effect를 적용해야 하는 경우 들어오는 분기입니다.
		for (AActor* TargetActor : CandidateTargetActors)
		{
			if (TargetActor)
			{
				OutTargetActors.AddUnique(TargetActor);
			}
		}
		return;
	}

	// TargetActorIndex번째 TargetActor에게 Effect를 적용하는 정책인 경우, SourceTargetActors에서 가져와 Out배열에 추가합니다.
	for (const int32 TargetActorIndex : EffectTargetMappingPolicy.TargetActorIndices)
	{
		if (CandidateTargetActors.IsValidIndex(TargetActorIndex) && CandidateTargetActors[TargetActorIndex])
		{
			OutTargetActors.AddUnique(CandidateTargetActors[TargetActorIndex]);
		}
	}
}

void ULetheCardAbility::GetEffectSpecBuildersByPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, TArray<const FGameplayEffectSpecBuilder*>& OutEffectSpecBuilders) const
{
	for (const auto& EffectSpecBuilder : EffectSpecBuilders)
	{
		const FGameplayEffectSpecBuilder* EffectSpecBuilderPtr = EffectSpecBuilder.GetPtr();
		if (!EffectSpecBuilderPtr)
		{
			continue;
		}

		if (EffectTargetMappingPolicy.EffectSpecBuilderTags.HasTagExact(EffectSpecBuilderPtr->GetEffectSpecBuilderTag()))
		{
			OutEffectSpecBuilders.AddUnique(EffectSpecBuilderPtr);
		}
	}
}

void ULetheCardAbility::StartDeliveryEffects(AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& SpecHandles) const
{
	if (SpecHandles.IsEmpty())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	if (!SourceASC)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	// EffectDelivery가 비어있다면 FEffectDelivery_Immediately를 사용하도록 fallback합니다.
	const FGameplayEffectDelivery* EffectDeliveryPtr = EffectDelivery.GetPtr();
	if (!EffectDeliveryPtr)
	{
		static const FEffectDelivery_Immediately DefaultDelivery;
		EffectDeliveryPtr = &DefaultDelivery;
	}

	FEffectDeliveryContext EffectDeliveryContext;
	EffectDeliveryContext.EffectSpecHandles = SpecHandles;
	EffectDeliveryContext.OwnerAbility = this;
	EffectDeliveryContext.SourceASC = SourceASC;
	EffectDeliveryContext.TargetASC = TargetASC;
	EffectDeliveryPtr->StartDelivery(EffectDeliveryContext);
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
	ResetCachedValues();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void ULetheCardAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateCancelAbility)
{
	// 프로젝트 특성상 한 번 발동된 Ability가 Cancel될 수는 없으나 일단 구현해두었습니다.
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
