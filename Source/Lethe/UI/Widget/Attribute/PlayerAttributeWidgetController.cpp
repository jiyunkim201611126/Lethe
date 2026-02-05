// Copyright JETBLU, Inc. All Rights Reserved.


#include "PlayerAttributeWidgetController.h"

#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

void UPlayerAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	Super::BindCallbacks(ASC, AS);
	
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnMaxManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);

	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		LethePlayerController->OnCardSelectedDelegate.AddUObject(this, &ThisClass::OnCardSelected);
		LethePlayerController->OnCancelCardSelectDelegate.AddUObject(this, &ThisClass::OnCancelCardSelect);
	}
}

void UPlayerAttributeWidgetController::BroadcastInitialValue()
{
	Super::BroadcastInitialValue();
	if (!AbilitySystemReferences.IsEmpty())
	{
		if (const ULetheAttributeSet* LetheAttributeSet = AbilitySystemReferences[0].AttributeSet)
		{
			OnManaChangedDelegate.Broadcast(LetheAttributeSet->GetMana());
			OnMaxManaChangedDelegate.Broadcast(LetheAttributeSet->GetMaxMana());
			OnCostChangedDelegate.Broadcast(LetheAttributeSet->GetCost());
		}
	}
}

void UPlayerAttributeWidgetController::OnManaChanged(const FOnAttributeChangeData& Data) const
{
	OnManaChangedDelegate.Broadcast(Data.NewValue);
}

void UPlayerAttributeWidgetController::OnMaxManaChanged(const FOnAttributeChangeData& Data) const
{
	OnMaxManaChangedDelegate.Broadcast(Data.NewValue);
}

void UPlayerAttributeWidgetController::OnCostChanged(const FOnAttributeChangeData& Data) const
{
	OnCostChangedDelegate.Broadcast(Data.NewValue);
}

void UPlayerAttributeWidgetController::OnCardSelected(const ULetheAbilitySystemComponent* CardOwnerASC, const ULetheGameplayAbility* CardAbility)
{
	if (!AbilitySystemReferences.IsEmpty() && AbilitySystemReferences[0].AbilitySystemComponent == CardOwnerASC)
	{
		// 해당 WidgetController와 관련 있는 ASC의 카드가 선택된 경우 들어오는 분기입니다.
		SelectedCardAbility = CardAbility;

		TMap<FGameplayAttribute, float> TempAbilityCostPreviewData;
		if (SelectedCardAbility->TryGetAbilityCostEffectPreviewData(TempAbilityCostPreviewData))
		{
			for (const auto& AbilityCostPreview : TempAbilityCostPreviewData)
			{
				if (const FGameplayTag* AttributeTag = AbilitySystemReferences[0].AttributeSet->AttributesToTags.Find(AbilityCostPreview.Key))
				{
					// 카드 Ability의 Cost Effect가 적용됐을 때 수정되는 Attribute를 멤버변수에 캐싱합니다.
					AbilityCostPreviewData.Emplace(*AttributeTag, AbilityCostPreview.Value);

					// 변경될 값을 Widget에 알려줍니다.
					if (const FOnAttributeChanged* Delegate = OnPreviewDataDelegateMap.Find(*AttributeTag))
					{
						Delegate->Broadcast(AbilityCostPreview.Value);
					}
				}
			}
		}
	}
}

void UPlayerAttributeWidgetController::OnCancelCardSelect()
{
	Super::OnCancelCardSelect();
	
	// 캐싱했던 값을 비워줍니다.
	AbilityCostPreviewData.Empty();
}
