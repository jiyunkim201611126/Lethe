// Copyright JETBLU, Inc. All Rights Reserved.

#include "OverlayWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void UOverlayWidgetController::BindCallbacksToDependencies(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	// Attribute들에게 변동사항이 있는 경우 Widget Controller가 알 수 있도록 각 AttributeSet에게 함수를 바인드합니다.
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddLambda([this, ASC](const FOnAttributeChangeData& Data)
		{
			OnHealthChangedDelegate.Broadcast(ASC, Data.NewValue);
		});

	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddLambda([this, ASC](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChangedDelegate.Broadcast(ASC, Data.NewValue);
		});
}

void UOverlayWidgetController::BroadcastInitialValue()
{
	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		OnHealthChangedDelegate.Broadcast(AbilitySystemReference.AbilitySystemComponent, AbilitySystemReference.AttributeSet->GetHealth());
		OnMaxHealthChangedDelegate.Broadcast(AbilitySystemReference.AbilitySystemComponent, AbilitySystemReference.AttributeSet->GetMaxHealth());
	}
}
