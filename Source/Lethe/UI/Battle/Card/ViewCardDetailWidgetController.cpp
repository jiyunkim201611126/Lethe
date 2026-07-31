// Copyright JETBLU, Inc. All Rights Reserved.

#include "ViewCardDetailWidgetController.h"

#include "GameplayAbilitySpecHandle.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Manager/LetheTextManager.h"

void UViewCardDetailWidgetController::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayAbilitySpecHandle AbilitySpecHandle, FText& OutDescription) const
{
	if (!OwnerASC)
	{
		return;
	}

	const FGameplayAbilitySpec* AbilitySpec = OwnerASC->FindAbilitySpecFromHandle(AbilitySpecHandle);
	if (!AbilitySpec)
	{
		return;
	}

	const ULetheCardAbility* CardAbility = Cast<ULetheCardAbility>(AbilitySpec->Ability);
	const UCardDefinitionData* CardDefinitionData = Cast<UCardDefinitionData>(AbilitySpec->SourceObject);
	if (!CardAbility || !CardDefinitionData)
	{
		return;
	}
	
	FText OutCardDescription;
	CardAbility->GetCardDescription(OwnerASC, AbilitySpec->Level, OutCardDescription);
	
	const FString WeightDescriptionKey = TEXT("Weight");
	const FText CardWeightDescription = FLetheTextManager::GetText(EStringTableType::CardDescription, WeightDescriptionKey, CardDefinitionData->CardWeight);
	
	OutDescription = FText::Join(INVTEXT("\n"), OutCardDescription, CardWeightDescription);
}
