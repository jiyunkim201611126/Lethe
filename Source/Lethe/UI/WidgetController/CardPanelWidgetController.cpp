// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/CardViewData.h"

void UCardPanelWidgetController::BindCallbacksToDependencies()
{
	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		AbilitySystemReference.AbilitySystemComponent->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);
	}
}

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& InAbilityTag) const
{
	const FCardViewInfo* CardViewInfo = CardViewData->FindCardInfoByTag(InAbilityTag);
	OnAbilityUpdatedDelegate.ExecuteIfBound(OwnerASC, CardViewInfo);
}
