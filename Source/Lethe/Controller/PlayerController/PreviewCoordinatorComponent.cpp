// Copyright JETBLU, Inc. All Rights Reserved.

#include "PreviewCoordinatorComponent.h"

#include "AbilitySystemInterface.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
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
	
	TMap<FGameplayAttribute, float> OutAbilityCostPreviewData;
	if (PreviewContext.SelectedCardAbility->TryGetAbilityCostEffectPreviewData(PreviewContext.SourceASC, OutAbilityCostPreviewData))
	{
		ConvertAttributeToTag(OutAbilityCostPreviewData, PreviewData.OutPreviewDataForSourceActor);
	}
	
	const IAbilitySystemInterface* CurrentTargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(PreviewContext.CurrentTargetActor);
	UAbilitySystemComponent* TargetASC = CurrentTargetAbilitySystemInterface ? CurrentTargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (TargetASC)
	{
		TMap<FGameplayAttribute, float> OutAbilityEffectsForSourcePreviewData;
		if (PreviewContext.SelectedCardAbility->TryGetAbilityEffectsForSourcePreviewData(PreviewContext.SourceASC, TargetASC, OutAbilityEffectsForSourcePreviewData))
		{
			ConvertAttributeToTag(OutAbilityEffectsForSourcePreviewData, PreviewData.OutPreviewDataForSourceActor);
		}
		TMap<FGameplayAttribute, float> OutAbilityEffectsForTargetPreviewData;
		if (PreviewContext.SelectedCardAbility->TryGetAbilityEffectsForTargetPreviewData(PreviewContext.SourceASC, TargetASC, OutAbilityEffectsForTargetPreviewData))
		{
			ConvertAttributeToTag(OutAbilityEffectsForTargetPreviewData, PreviewData.OutPreviewDataForTargetActor);
		}
	}

	if (OnPreviewDataUpdated.IsBound())
	{
		OnPreviewDataUpdated.Broadcast(PreviewContext, PreviewData);
	}
}

void UPreviewCoordinatorComponent::ConvertAttributeToTag(const TMap<FGameplayAttribute, float>& InMap, TMap<FGameplayTag, float>& OutMap) const
{
	for (const auto& Elem : InMap)
	{
		if (const FGameplayTag* AttributeTag = ULetheAttributeSet::AttributesToTags.Find(Elem.Key))
		{
			const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
			if (AttributeTag->MatchesTag(LetheGameplayTags.Attributes_Meta_IncomingDamage))
			{
				// Damage Attribute인 경우 Health Attribute로 바꾸고, 값도 음수로 바꿔서 넣어줍니다.
				OutMap.FindOrAdd(LetheGameplayTags.Attributes_Vital_Health) -= Elem.Value;
			}
			else
			{
				// Cost와 마우스 Hovered 상태를 합산해서 보여주기 위해 Emplace가 아닌 FindOrAdd를 사용합니다.
				OutMap.FindOrAdd(*AttributeTag) += Elem.Value;
			}
		}
	}
}
