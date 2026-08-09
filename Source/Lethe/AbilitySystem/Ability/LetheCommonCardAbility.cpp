// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCommonCardAbility.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/EffectDelivery/EffectDelivery_Immediately.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/LetheTextManager.h"

FEffectTargetMappingPolicy::FEffectTargetMappingPolicy()
{
	MontageEventTag = FGameplayTag::RequestGameplayTag(FName("Event.Montage.1"));
	EffectSpecBuilderTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Event.EffectSpecBuilder.1"), false));
	TargetGroupTags.AddTag(FGameplayTag::RequestGameplayTag(FName("TargetGroup.Primary"), false));
}

ULetheCommonCardAbility::ULetheCommonCardAbility()
{
	EffectDelivery.InitializeAs<FEffectDelivery_Immediately>();
}

bool ULetheCommonCardAbility::TryGetEffectsForSourceAndTargetPreviewData(UAbilitySystemComponent* SourceASC, const TArray<FTargetSelectionResult>& TargetSelectionResults, FGameplayEffectPreviewData& OutPreviewData) const
{
	OutPreviewData.SourcePreviewData.Reset();
	OutPreviewData.TargetPreviewData.Reset();
	if (!SourceASC)
	{
		return false;
	}

	for (const FEffectTargetMappingPolicy& EffectTargetMappingPolicy : EffectTargetMappingPolicies)
	{
		FEffectTargetMappingResolveResult OutResolveResult;
		ResolveEffectTargetMappingPolicy(EffectTargetMappingPolicy, SourceASC, TargetSelectionResults, OutResolveResult);

		TryGetGameplayEffectPreviewData(SourceASC, OutResolveResult.SourceSpecHandles, OutPreviewData.SourcePreviewData);

		for (const auto& Pair : OutResolveResult.TargetSpecHandlesByActor)
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Pair.Key))
			{
				TMap<FGameplayAttribute, float>& OutPreviewDataForTarget = OutPreviewData.TargetPreviewData.FindOrAdd(TargetASC);
				TryGetGameplayEffectPreviewData(TargetASC, Pair.Value, OutPreviewDataForTarget);
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

void ULetheCommonCardAbility::GetCardDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, FText& OutDescription) const
{
	OutDescription = FText();
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	TArray<FText> OutTexts;
	for (const FEffectTargetMappingPolicy& EffectTargetMappingPolicy : EffectTargetMappingPolicies)
	{
		FString LocalTextKey;
		if (EffectTargetMappingPolicy.bApplyToAllTargets)
		{
			// 모든 대상에게 적용하는 경우 들어오는 분기입니다.
			LocalTextKey = TEXT("TargetGroup.All");
		}
		else
		{
			if (EffectTargetMappingPolicy.TargetGroupTags.HasTagExact(LetheGameplayTags.TargetGroup_Penetration) &&
				EffectTargetMappingPolicy.TargetGroupTags.HasTagExact(LetheGameplayTags.TargetGroup_HalfMoon) &&
				EffectTargetMappingPolicy.TargetGroupTags.HasTagExact(LetheGameplayTags.TargetGroup_Spread))
			{
				// 모든 추가 선택 대상에게 적용하는 경우 들어오는 분기입니다.
				LocalTextKey = TEXT("TargetGroup.Extra");
			}
			else
			{
				// 그 외의 경우에 들어오는 분기입니다.
				LocalTextKey = EffectTargetMappingPolicy.TargetGroupTags.First().ToString();
			}
		}
		
		const FText TargetText = FLetheTextManager::GetText(EStringTableType::CardDescription, LocalTextKey);
		
		// 한 대상 그룹에게 여러 이펙트를 적용할 수 있으므로, EffectSpecBuilders를 순회하며 TargetText와 합친 텍스트를 생성합니다.
		TArray<const FGameplayEffectSpecBuilder*> OutEffectSpecBuilders;
		GetEffectSpecBuildersByPolicy(EffectTargetMappingPolicy, OutEffectSpecBuilders);
		for (const FGameplayEffectSpecBuilder* EffectSpecBuilder : OutEffectSpecBuilders)
		{
			FText EffectText;
			EffectSpecBuilder->GetDescription(OwnerASC, InLevel, EffectText);
			
			if (!EffectText.IsEmpty())
			{
				OutTexts.Add(FText::Format(INVTEXT("{0} {1}"), TargetText, EffectText));
			}
		}
	}
	
	// 줄바꿈된 형태로 Out 인자에 넣어줍니다.
	OutDescription = FText::Join(INVTEXT("\n"), OutTexts);
}

void ULetheCommonCardAbility::RegisterAbilityEventTasks()
{
	// 갖고 있는 모든 EffectTargetMappingPolicy의 MontageEventTag로 WaitGameplayEvent Task를 생성합니다.
	TSet<FGameplayTag> RegisteredEventTags;
	for (const FEffectTargetMappingPolicy& EffectTargetMappingPolicy : EffectTargetMappingPolicies)
	{
		if (!EffectTargetMappingPolicy.MontageEventTag.IsValid() || RegisteredEventTags.Contains(EffectTargetMappingPolicy.MontageEventTag))
		{
			continue;
		}

		// MontageEvent를 받으면 다시 모든 Policy를 순회하며 동일 Tag를 탐색하므로 하나만 수행합니다.
		RegisteredEventTags.Add(EffectTargetMappingPolicy.MontageEventTag);
		
		UAbilityTask_WaitGameplayEvent* WaitApplyEffectEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			EffectTargetMappingPolicy.MontageEventTag,
			nullptr,
			true,
			true);
		WaitApplyEffectEventTask->EventReceived.AddDynamic(this, &ThisClass::OnEventReceived);
		WaitApplyEffectEventTask->ReadyForActivation();
	}
}

void ULetheCommonCardAbility::HandleAbilityEvent(const FGameplayEventData& InPayload)
{
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
			FEffectTargetMappingResolveResult OutResolveResult;
			ResolveEffectTargetMappingPolicy(EffectTargetMappingPolicy, SourceASC, CachedTargetSelectionResults, OutResolveResult);

			for (const FGameplayEffectSpecHandle& SourceSpecHandle : OutResolveResult.SourceSpecHandles)
			{
				if (SourceSpecHandle.IsValid())
				{
					SourceASC->ApplyGameplayEffectSpecToSelf(*SourceSpecHandle.Data.Get());
				}
			}

			TArray<AActor*> AffectedTargetActors;
			for (const auto& Pair : OutResolveResult.TargetSpecHandlesByActor)
			{
				if (Pair.Key)
				{
					AffectedTargetActors.Add(Pair.Key);
					StartDeliveryEffects(Pair.Key, Pair.Value);
				}
			}

			if (!AffectedTargetActors.IsEmpty())
			{
				OnEffectTriggered(EffectTargetMappingPolicy.MontageEventTag, AffectedTargetActors);
			}
		}
	}
}

void ULetheCommonCardAbility::ResolveEffectTargetMappingPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, UAbilitySystemComponent* SourceASC, const TArray<FTargetSelectionResult>& TargetSelectionResults, FEffectTargetMappingResolveResult& OutResult) const
{
	OutResult.SourceSpecHandles.Reset();
	OutResult.TargetSpecHandlesByActor.Reset();

	const bool bHasValidTargetGroupPolicy = EffectTargetMappingPolicy.bApplyToAllTargets || !EffectTargetMappingPolicy.TargetGroupTags.IsEmpty();
	if (!ensure(bHasValidTargetGroupPolicy) || !SourceASC)
	{
		return;
	}

	// Policy에 해당하는 EffectSpecBuilder를 가져옵니다.
	TArray<const FGameplayEffectSpecBuilder*> OutEffectSpecBuilders;
	GetEffectSpecBuildersByPolicy(EffectTargetMappingPolicy, OutEffectSpecBuilders);

	if (OutEffectSpecBuilders.IsEmpty())
	{
		return;
	}

	// Policy에 해당하는 TargetActors를 가져옵니다.
	// TargetActors가 없더라도, '자기 자신에게 사용'하는 카드일 수 있으므로 얼리리턴하지 않습니다.
	TArray<AActor*> TargetActors;
	for (const FTargetSelectionResult& TargetResult : TargetSelectionResults)
	{
		if (EffectTargetMappingPolicy.bApplyToAllTargets || EffectTargetMappingPolicy.TargetGroupTags.HasTagExact(TargetResult.TargetGroupTag))
		{
			for (const auto& Target : TargetResult.Targets)
			{
				if (Target.ActorOnTile.IsValid())
				{
					TargetActors.AddUnique(Target.ActorOnTile.Get());
				}
			}
		}
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

		for (AActor* TargetActor : TargetActors)
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

void ULetheCommonCardAbility::GetEffectSpecBuildersByPolicy(const FEffectTargetMappingPolicy& EffectTargetMappingPolicy, TArray<const FGameplayEffectSpecBuilder*>& OutEffectSpecBuilders) const
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

void ULetheCommonCardAbility::StartDeliveryEffects(AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& SpecHandles) const
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
