// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidgetController.h"

#include "AbilitySystemInterface.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Controller/PlayerController/PreviewCoordinatorComponent.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UPlayerAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	Super::BindCallbacks(ASC, AS);
	
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);
}

void UPlayerAttributeWidgetController::OnManaChanged(const FOnAttributeChangeData& AttributeData)
{
	UpdateCachedAttribute(AttributeData);
	BroadcastManaChanged();
}

void UPlayerAttributeWidgetController::BroadcastManaChanged() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FAttributeData Data;
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Mana);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxMana);
	if (const FOnAttributeChanged* OnManaChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Mana))
	{
		OnManaChanged->Broadcast(Data);
	}
}

void UPlayerAttributeWidgetController::OnCostChanged(const FOnAttributeChangeData& AttributeData)
{
	UpdateCachedAttribute(AttributeData);
	BroadcastCostChanged();
}

void UPlayerAttributeWidgetController::BroadcastCostChanged() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FAttributeData Data;
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Cost);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxCost);
	if (const FOnAttributeChanged* OnCostChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Cost))
	{
		OnCostChanged->Broadcast(Data);
	}
}

void UPlayerAttributeWidgetController::OnPreviewDataUpdated(const FPreviewContext& PreviewContext, const FPreviewData& PreviewData)
{
	if (AbilitySystemReferences.IsEmpty())
	{
		return;
	}

	const UAbilitySystemComponent* ThisASC = AbilitySystemReferences[0].AbilitySystemComponent;
	
	const IAbilitySystemInterface* CurrentTargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(PreviewContext.CurrentTargetActor);
	const UAbilitySystemComponent* CurrentTargetASC = CurrentTargetAbilitySystemInterface ? CurrentTargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

	StopAllPreview();

	// 선택된 카드의 주인이면서 동시에 Target으로 지정된 ASC면 들어가는 분기입니다.
	if (PreviewContext.SourceASC == ThisASC && CurrentTargetASC == ThisASC)
	{
		if (!PreviewData.OutPreviewDataForSourceActor.IsEmpty() || !PreviewData.OutPreviewDataForTargetActor.IsEmpty())
		{
			TMap<FGameplayTag, float> AllPreviewData;
			for (const auto& Elem : PreviewData.OutPreviewDataForSourceActor)
			{
				AllPreviewData.FindOrAdd(Elem.Key) += Elem.Value;
			}
			for (const auto& Elem : PreviewData.OutPreviewDataForTargetActor)
			{
				AllPreviewData.FindOrAdd(Elem.Key) += Elem.Value;
			}
			StartAllPreview(AllPreviewData);
		}
		return;
	}
	
	// 선택된 Card의 OwnerASC가 이 WidgetController가 관찰 중인 ASC면 들어가는 분기입니다.
	if (PreviewContext.SourceASC == ThisASC)
	{
		if (!PreviewData.OutPreviewDataForSourceActor.IsEmpty())
		{
			StartAllPreview(PreviewData.OutPreviewDataForSourceActor);
		}
	}

	// CurrentActor의 ASC가 이 WidgetController가 관찰 중인 ASC면 들어가는 분기입니다.
	if (CurrentTargetASC == ThisASC)
	{
		if (!PreviewData.OutPreviewDataForTargetActor.IsEmpty())
		{
			StartAllPreview(PreviewData.OutPreviewDataForTargetActor);
		}
	}
}

void UPlayerAttributeWidgetController::StartAllPreview(const TMap<FGameplayTag, float>& InPreviewData)
{
	Super::StartAllPreview(InPreviewData);
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StartPreview(LetheGameplayTags.Attributes_Vital_Mana, LetheGameplayTags.Attributes_Vital_MaxMana, InPreviewData);
	StartPreview(LetheGameplayTags.Attributes_Vital_Cost, LetheGameplayTags.Attributes_Vital_MaxCost, InPreviewData);
}

void UPlayerAttributeWidgetController::StopAllPreview()
{
	Super::StopAllPreview();

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StopPreview(LetheGameplayTags.Attributes_Vital_Mana, LetheGameplayTags.Attributes_Vital_MaxMana);
	StopPreview(LetheGameplayTags.Attributes_Vital_Cost, LetheGameplayTags.Attributes_Vital_MaxCost);
}
