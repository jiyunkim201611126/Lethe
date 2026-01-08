// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardViewData.h"

void UCardPanelWidgetController::BindCallbacksToDependencies()
{
	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		AbilitySystemReference.AbilitySystemComponent->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);
	}
}

FVector2D UCardPanelWidgetController::GetCardSize() const
{
	return CardViewData->CardHighlightSize;
}

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, ULetheGameplayAbility* InAbility) const
{
	// AbilityTag와 Ability를 통해 Card의 View를 초기화하기 위한 값들을 가져와 델리게이트를 통해 뿌립니다.
	FCardViewInfo* CardViewInfo = CardViewData->FindCardInfoByTag(InAbility->AbilityTag);
	const FText CardNameText = InAbility->GetCardName();
	const FText CardDescriptionText = InAbility->GetCardDescription(InAbility->GetAbilityLevel());
	CardViewInfo->CardNameText = CardNameText;
	CardViewInfo->CardDescriptionText = CardDescriptionText;
	OnAbilityUpdatedDelegate.ExecuteIfBound(OwnerASC, CardViewInfo);
}
