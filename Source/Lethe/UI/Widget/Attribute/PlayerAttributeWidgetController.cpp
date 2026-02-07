// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidgetController.h"

#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UPlayerAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	Super::BindCallbacks(ASC, AS);
	
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);
	
	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		LethePlayerController->OnCardSelectedDelegate.AddUObject(this, &ThisClass::OnCardSelected);
		LethePlayerController->OnCancelCardSelectDelegate.AddUObject(this, &ThisClass::OnCancelCardSelect);
	}
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
	Data.bIsPreview = false;
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
	Data.bIsPreview = false;
	Data.CurrentValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_Cost);
	Data.MaxValue = CachedAttribute.FindRef(LetheGameplayTags.Attributes_Vital_MaxCost);
	if (const FOnAttributeChanged* OnCostChanged = OnAttributeChangedMap.Find(LetheGameplayTags.Attributes_Vital_Cost))
	{
		OnCostChanged->Broadcast(Data);
	}
}

void UPlayerAttributeWidgetController::OnCardSelected(const ULetheAbilitySystemComponent* CardOwnerASC, const ULetheGameplayAbility* CardAbility)
{
	if (AbilitySystemReferences.IsValidIndex(0) && AbilitySystemReferences[0].AbilitySystemComponent == CardOwnerASC && AbilitySystemReferences[0].AttributeSet)
	{
		// 해당 WidgetController와 관련 있는 ASC의 카드가 선택된 경우 들어오는 분기입니다.
		SelectedCardAbility = CardAbility;

		// 카드 Ability의 Cost Effect가 적용됐을 때 변경되는 Attribute 값을 가져옵니다.
		TMap<FGameplayAttribute, float> TempAbilityCostPreviewData;
		if (SelectedCardAbility->TryGetAbilityCostEffectPreviewData(TempAbilityCostPreviewData))
		{
			for (const auto& AbilityCostPreview : TempAbilityCostPreviewData)
			{
				if (const FGameplayTag* AttributeTag = AbilitySystemReferences[0].AttributeSet->AttributesToTags.Find(AbilityCostPreview.Key))
				{
					// 변경될 Attribute와 그 값을 캐싱합니다.
					CachedPreviewAttribute.Emplace(*AttributeTag, AbilityCostPreview.Value);
				}
			}

			StartAllPreview();
		}
	}
}

void UPlayerAttributeWidgetController::OnCancelCardSelect()
{
	StopAllPreview();
	
	// 관련 데이터를 모두 제거합니다.
	SelectedCardAbility = nullptr;
	CachedPreviewAttribute.Empty();
}

void UPlayerAttributeWidgetController::StartAllPreview()
{
	Super::StartAllPreview();
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StartPreview(LetheGameplayTags.Attributes_Vital_Mana, LetheGameplayTags.Attributes_Vital_MaxMana);
	StartPreview(LetheGameplayTags.Attributes_Vital_Cost, LetheGameplayTags.Attributes_Vital_MaxCost);
}

void UPlayerAttributeWidgetController::StopAllPreview()
{
	Super::StopAllPreview();

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	StopPreview(LetheGameplayTags.Attributes_Vital_Mana, LetheGameplayTags.Attributes_Vital_MaxMana);
	StopPreview(LetheGameplayTags.Attributes_Vital_Cost, LetheGameplayTags.Attributes_Vital_MaxCost);
}
