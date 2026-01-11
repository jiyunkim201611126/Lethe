// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"

void UCardPanelWidgetController::BindCallbacksToDependencies(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	ASC->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);
}

FVector2D UCardPanelWidgetController::GetCardSize() const
{
	return CardViewData->GetCardSize();
}

float UCardPanelWidgetController::GetCardHighlightScale() const
{
	return CardViewData->GetCardHighlightScale();
}

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, ULetheGameplayAbility* InAbility) const
{
	// Ability에서 CardDescription을 가져와 DataAsset에 넣어줍니다.
	FCardViewInfo* CardViewInfo = CardViewData->FindCardInfoByTag(InAbility->CardTag);
	if (CardViewInfo)
	{
		const FText CardDescriptionText = InAbility->GetCardDescription(InAbility->GetAbilityLevel());
		CardViewInfo->CardDescriptionText = CardDescriptionText;
	}

	// 콜백을 받은 CardPanelWidget이 필요한 데이터를 참조할 수 있도록 보내줍니다.
	if (IPlayableCharacterInterface* OwnerCharacter = Cast<IPlayableCharacterInterface>(OwnerASC->GetOwner()))
	{
		const FCardInitParams InitParams(OwnerASC, CardViewData, InAbility->CardTag, OwnerCharacter->GetCardFrontsideColor(), OwnerCharacter->GetCardBacksideColor());
		OnAbilityUpdatedDelegate.ExecuteIfBound(InitParams);
	}
}
