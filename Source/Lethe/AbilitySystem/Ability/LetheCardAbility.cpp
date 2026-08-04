// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCardAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/EffectTargetTileSelector/EffectTargetTileSelector.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void ULetheCardAbility::GetCandidateTiles(FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
	FillTileSelectorContext(Context);
	OutResult.Reset();
	if (const FEffectTargetTileSelector* EffectTargetTileSelectorPtr = EffectTargetTileSelector.GetPtr())
	{
		EffectTargetTileSelectorPtr->GetCandidateTiles(Context, OutResult);
	}
}

void ULetheCardAbility::GetTargetTiles(FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
	FillTileSelectorContext(Context);
	OutResult.Reset();
	if (const FEffectTargetTileSelector* EffectTargetTileSelectorPtr = EffectTargetTileSelector.GetPtr())
	{
		EffectTargetTileSelectorPtr->GetTargetTiles(Context, OutResult);
	}
}

void ULetheCardAbility::GetTargetTilesForAI(FEffectTargetTileSelectorContext& Context, FEffectTargetTileSelectorResult& OutResult) const
{
	FillTileSelectorContext(Context);
	OutResult.Reset();
	if (const FEffectTargetTileSelector* EffectTargetTileSelectorPtr = EffectTargetTileSelector.GetPtr())
	{
		EffectTargetTileSelectorPtr->GetTargetTilesForAI(Context, OutResult);
	}
}

void ULetheCardAbility::FillTileSelectorContext(FEffectTargetTileSelectorContext& Context) const
{
	if (Context.AvatarActor)
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = Context.AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			Context.TileManagerSubsystem = TileManagerSubsystem;
			if (!Context.SourceTile)
			{
				if (const ATile* SourceTile = TileManagerSubsystem->GetTileUnderActor(Context.AvatarActor))
				{
					Context.SourceTile = SourceTile;
				}
			}
		}
	}
}

bool ULetheCardAbility::TryGetCostEffectPreviewData(UAbilitySystemComponent* SourceASC, TMap<FGameplayAttribute, float>& OutCostPreviewData) const
{
	OutCostPreviewData.Reset();
	if (CostGameplayEffectClass && SourceASC)
	{
		// Ability Cost는 Ability가 소유자이므로, EffectSpec을 직접 만들어서 Preview Data를 추출합니다.
		FGameplayEffectContextHandle PreviewContextHandle = SourceASC->MakeEffectContext();
		PreviewContextHandle.SetAbility(this);
		const FGameplayEffectSpecHandle CostEffectSpecHandle = SourceASC->MakeOutgoingSpec(CostGameplayEffectClass, 1.f, PreviewContextHandle);

		TArray<FGameplayEffectSpecHandle> CostEffectSpecHandleArray;
		CostEffectSpecHandleArray.Add(CostEffectSpecHandle);

		return TryGetGameplayEffectPreviewData(SourceASC, CostEffectSpecHandleArray, OutCostPreviewData);
	}
	return false;
}

bool ULetheCardAbility::TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<FTargetSelectionResult>& TargetSelectionResults, FGameplayEffectPreviewData& OutPreviewData) const
{
	// Preview가 불필요한 Ability도 존재할 수 있으므로 기본 구현은 false를 반환합니다.
	return false;
}

bool ULetheCardAbility::TryGetGameplayEffectPreviewData(UAbilitySystemComponent* PreviewTargetASC, const TArray<FGameplayEffectSpecHandle>& SpecHandles, TMap<FGameplayAttribute, float>& OutPreviewData) const
{
	if (!PreviewTargetASC)
	{
		return false;
	}
	
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
		HandleActivationFailed();
		return;
	}

	// 플레이어 캐릭터인 경우 소음 발생 로직을 시작합니다.
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			ATile* StandingTile = TileManagerSubsystem->GetTileUnderActor(AvatarActor);
			TArray<ATile*> NoiseTiles;
			for (const FTargetSelectionResult& TargetResult : CachedTargetSelectionResults)
			{
				for (const FSelectedTarget& Target : TargetResult.Targets)
				{
					NoiseTiles.Add(Target.TargetTile.Get());
				}
			}
			ActivateNoise(StandingTile, NoiseTiles);
		}
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();

	// 어떤 CardAbility를 사용하든, 한 번 사용하고 나면 해당 턴에서 더이상 움직일 수 없습니다.
	ActorInfo->AbilitySystemComponent->AddLooseGameplayTag(LetheGameplayTags.State_Character_MoveConsumed);

	// 자식 클래스들이 Task를 등록할 수 있도록 훅을 걸어둡니다.
	RegisterAbilityEventTasks();

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
	PlayMontageAndWaitTask->OnCancelled.AddDynamic(this, &ThisClass::HandleActivationFailed);
	PlayMontageAndWaitTask->OnInterrupted.AddDynamic(this, &ThisClass::HandleActivationFailed);
	PlayMontageAndWaitTask->ReadyForActivation();
}

void ULetheCardAbility::RegisterAbilityEventTasks()
{
}

void ULetheCardAbility::OnEventReceived(FGameplayEventData InPayload)
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (InPayload.EventTag.MatchesTagExact(LetheGameplayTags.Event_Montage_EndAbility))
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
		return;
	}
	
	// 자식 클래스들이 Task를 처리할 수 있도록 훅을 걸어둡니다.
	HandleAbilityEvent(InPayload);
}

void ULetheCardAbility::HandleAbilityEvent(const FGameplayEventData& InPayload)
{
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

	if (TargetData->GetScriptStruct() != FGameplayAbilityTargetData_TargetSelectionResults::StaticStruct())
	{
		return false;
	}

	const FGameplayAbilityTargetData_TargetSelectionResults* TargetSelectionResultsData = static_cast<const FGameplayAbilityTargetData_TargetSelectionResults*>(TargetData);
	CachedTargetSelectionResults = TargetSelectionResultsData->TargetSelectionResults;

	const AActor* SourceActor = GetAvatarActorFromActorInfo();
	if (!SourceActor)
	{
		ResetCachedValues();
		return false;
	}

	// SourceActor가 플레이어 캐릭터인 경우 들어가는 분기입니다.
	if (SourceActor->Implements<UPlayerCharacterInterface>())
	{
		// 플레이어 캐릭터인 경우에만 Cost 관련 로직을 수행합니다.
		if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
		{
			ResetCachedValues();
			return false;
		}
	}

	return true;
}

void ULetheCardAbility::HandleActivationFailed()
{
	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnAbilityActivationFailed();
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
}

void ULetheCardAbility::ResetCachedValues()
{
	CachedTargetSelectionResults.Empty();
}

void ULetheCardAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	ResetCachedValues();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
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

bool ULetheCardAbility::CanUseWithoutTarget() const
{
	return bCanUseWithoutTarget;
}

#if WITH_EDITOR
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
#endif
