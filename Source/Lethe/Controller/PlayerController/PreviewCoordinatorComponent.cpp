// Copyright JETBLU, Inc. All Rights Reserved.

#include "PreviewCoordinatorComponent.h"

#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/Data/PreviewData.h"
#include "Lethe/Manager/LetheGameplayTags.h"

UPreviewCoordinatorComponent::UPreviewCoordinatorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPreviewCoordinatorComponent::StartCalculatingPreviewData(const FPreviewContext& PreviewContext) const
{
	if (!PreviewContext.SourceASC || !PreviewContext.SelectedCardAbility)
	{
		return;
	}
	
	FPreviewData PreviewData;
	
	TMap<FGameplayAttribute, float> OutCostPreviewData;
	FGameplayEffectPreviewData OutPreviewData;
	
	// Target과 관계 없이 CostEffect에 의해 Source에게 적용되는 Preview 데이터를 추출해 가져옵니다.
	if (PreviewContext.SelectedCardAbility->TryGetCostEffectPreviewData(PreviewContext.SourceASC, OutCostPreviewData))
	{
		FAttributePreviewDelta& AttributePreviewDelta = PreviewData.ASCToPreviewData.FindOrAdd(PreviewContext.SourceASC);
		ConvertAttributeToTag(OutCostPreviewData, AttributePreviewDelta.AttributePreviewDelta);
	}

	// Target과 관계 없이 EffectApplier에 의해 Source에게 적용되는 Preview 데이터를 추출해 가져옵니다.
	if (PreviewContext.SelectedCardAbility->TryGetEffectsForSourcePreviewData(PreviewContext.SourceASC, OutPreviewData.SourcePreviewData))
	{
		FAttributePreviewDelta& AttributePreviewDeltaForSource = PreviewData.ASCToPreviewData.FindOrAdd(PreviewContext.SourceASC);
		ConvertAttributeToTag(OutPreviewData.SourcePreviewData, AttributePreviewDeltaForSource.AttributePreviewDelta);
	}

	TArray<AActor*> TargetActors;
	TargetActors.Reserve(PreviewContext.CurrentTargetActors.Num());
	for (const auto& TargetActor : PreviewContext.CurrentTargetActors)
	{
		if (TargetActor.IsValid() && TargetActor->Implements<UAbilitySystemInterface>())
		{
			TargetActors.Add(TargetActor.Get());
		}
	}
	
	// Target들에게 적용되는 Preview 데이터를 추출해 가져옵니다.
	OutPreviewData.SourcePreviewData.Empty();
	if (PreviewContext.SelectedCardAbility->TryGetEffectsForSourceAndTargetPreviewData(PreviewContext.SourceASC, TargetActors, OutPreviewData))
	{
		FAttributePreviewDelta& AttributePreviewDelta = PreviewData.ASCToPreviewData.FindOrAdd(PreviewContext.SourceASC);
		ConvertAttributeToTag(OutPreviewData.SourcePreviewData, AttributePreviewDelta.AttributePreviewDelta);
		
		for (const auto& TargetPreviewData : OutPreviewData.TargetPreviewData)
		{
			FAttributePreviewDelta& AttributePreviewDeltaForTarget = PreviewData.ASCToPreviewData.FindOrAdd(TargetPreviewData.Key);
			ConvertAttributeToTag(TargetPreviewData.Value, AttributePreviewDeltaForTarget.AttributePreviewDelta);
		}
	}

	if (OnUpdatePreviewData.IsBound())
	{
		OnUpdatePreviewData.Broadcast(PreviewData);
	}
}

void UPreviewCoordinatorComponent::StopAllPreview() const
{
	if (OnUpdatePreviewData.IsBound())
	{
		const FPreviewData PreviewData;
		OnUpdatePreviewData.Broadcast(PreviewData);
	}
}

void UPreviewCoordinatorComponent::ConvertAttributeToTag(const TMap<FGameplayAttribute, float>& InMap, TMap<FGameplayTag, float>& OutMap) const
{
	for (const auto& Elem : InMap)
	{
		if (const FGameplayTag* AttributeTag = ULetheAttributeSet::AttributesToTags.Find(Elem.Key))
		{
			const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
			if (AttributeTag->MatchesTag(LetheGameplayTags.Attribute_Meta_IncomingDamage))
			{
				// Damage Attribute인 경우 Health Attribute로 바꾸고, 값도 음수로 바꿔서 넣어줍니다.
				OutMap.FindOrAdd(LetheGameplayTags.Attribute_Vital_Health) -= Elem.Value;
			}
			else
			{
				// Cost와 마우스 Hovered 상태를 합산해서 보여주기 위해 Emplace가 아닌 FindOrAdd를 사용합니다.
				OutMap.FindOrAdd(*AttributeTag) += Elem.Value;
			}
		}
	}
}
