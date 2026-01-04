// Copyright JETBLU, Inc. All Rights Reserved.

#include "OverlayWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void UOverlayWidgetController::BindCallbacksToDependencies()
{
	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		// Attribute들에게 변동사항이 있는 경우 Widget Controller가 알 수 있도록 각 AttributeSet에게 함수를 바인드합니다.
		AbilitySystemReference.AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AbilitySystemReference.AttributeSet->GetHealthAttribute()).AddLambda([this, AbilitySystemReference](const FOnAttributeChangeData& Data)
			{
				OnHealthChanged.Broadcast(AbilitySystemReference.AbilitySystemComponent, Data.NewValue);
			});
	
		AbilitySystemReference.AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(AbilitySystemReference.AttributeSet->GetMaxHealthAttribute()).AddLambda([this, AbilitySystemReference](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChanged.Broadcast(AbilitySystemReference.AbilitySystemComponent, Data.NewValue);
			});
	}
}

void UOverlayWidgetController::BroadcastInitialValue()
{
	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		OnHealthChanged.Broadcast(AbilitySystemReference.AbilitySystemComponent, AbilitySystemReference.AttributeSet->GetHealth());
		OnMaxHealthChanged.Broadcast(AbilitySystemReference.AbilitySystemComponent, AbilitySystemReference.AttributeSet->GetMaxHealth());
	}
}
