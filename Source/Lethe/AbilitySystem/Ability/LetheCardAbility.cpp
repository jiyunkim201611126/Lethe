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
#include "Lethe/Manager/LetheTextManager.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void ULetheCardAbility::GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles) const
{
	if (const FEffectTargetTileSelector* EffectTargetTileSelectorPtr = EffectTargetTileSelector.GetPtr())
	{
		EffectTargetTileSelectorPtr->GetCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles, OutTargetCandidateTiles);
	}
}

void ULetheCardAbility::GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	if (const FEffectTargetTileSelector* EffectTargetTileSelectorPtr = EffectTargetTileSelector.GetPtr())
	{
		EffectTargetTileSelectorPtr->GetTargetTiles(AvatarActor, PlayerController, OutTiles);
	}
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
	return false;
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

	// 플레이어 캐릭터인 경우 소음 발생 로직을 시작합니다.
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (AvatarActor && AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			const ATile* StandingTile = TileManagerSubsystem->GetTileUnderActor(AvatarActor);
			ActivateNoise(StandingTile, CachedCenterTargetTile.Get());
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
	PlayMontageAndWaitTask->OnCancelled.AddDynamic(this, &ThisClass::ActiveFailed);
	PlayMontageAndWaitTask->OnInterrupted.AddDynamic(this, &ThisClass::ActiveFailed);
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
		if (!CommitAbilityCost(Handle, ActorInfo, ActivationInfo))
		{
			ResetCachedValues();
			return false;
		}

		return true;
	}

	// SourceActor가 적 캐릭터인 경우 이곳으로 내려옵니다.
	// 이 경우 Target Tile 위에 캐릭터가 없더라도, 애니메이션이나 나이아가라를 재생하기 위해 더미 액터를 올려두어 진행하기 때문에 true를 반환합니다.
	return true;
}

void ULetheCardAbility::ActiveFailed()
{
	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnAbilityActivationFailed();
	}
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, false, false);
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
