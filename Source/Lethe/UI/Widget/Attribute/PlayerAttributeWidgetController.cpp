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

	Mana = AS->GetMana();
	MaxMana = AS->GetMaxMana();
	Cost = AS->GetCost();

	if (ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
	{
		LethePlayerController->OnCardSelectedDelegate.AddUObject(this, &ThisClass::OnCardSelected);
	}
}

void UPlayerAttributeWidgetController::BroadcastInitialValue()
{
	Super::BroadcastInitialValue();
	OnManaChangedDelegate.Broadcast(Mana, MaxMana);
	OnCostChangedDelegate.Broadcast(Cost);
}

void UPlayerAttributeWidgetController::OnManaChanged(const FOnAttributeChangeData& Data)
{
	Mana = Data.NewValue;
	OnManaChangedDelegate.Broadcast(Mana, MaxMana);
}

void UPlayerAttributeWidgetController::OnMaxManaChanged(const FOnAttributeChangeData& Data)
{
	MaxMana = Data.NewValue;
	OnManaChangedDelegate.Broadcast(Mana, MaxMana);
}

void UPlayerAttributeWidgetController::OnCostChanged(const FOnAttributeChangeData& Data)
{
	Cost = Data.NewValue;
	OnCostChangedDelegate.Broadcast(Cost);
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
					AbilityCostPreviewData.Emplace(*AttributeTag, AbilityCostPreview.Value);
			
					if (const FOnPreviewValueChanged* Delegate = OnPreviewDataChangedMap.Find(*AttributeTag))
					{
						Delegate->Broadcast(AbilityCostPreview.Value);
					}
				}
			}
		}
	}
}
