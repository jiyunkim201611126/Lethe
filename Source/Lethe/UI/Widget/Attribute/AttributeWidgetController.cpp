// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void UAttributeWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	PlayerController = WidgetControllerParams.PlayerController;
	PlayerState = WidgetControllerParams.PlayerState;

	// AttributeWidget과 1:1 대응되는 WidgetController이므로, 1개만 있으면 됩니다.
	AbilitySystemReferences.Reserve(1);
	ULetheAbilitySystemComponent* AbilitySystemComponent = Cast<ULetheAbilitySystemComponent>(WidgetControllerParams.AbilitySystemComponent);
	ULetheAttributeSet* AttributeSet = Cast<ULetheAttributeSet>(WidgetControllerParams.AttributeSet);
	AbilitySystemReferences.Emplace(FAbilitySystemReference(AbilitySystemComponent, AttributeSet));
}

void UAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	// Attribute들에게 변동사항이 있는 경우 Widget Controller가 알 수 있도록 각 AttributeSet에게 함수를 바인드합니다.
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthChanged);
}

void UAttributeWidgetController::BroadcastInitialValue()
{
	if (!AbilitySystemReferences.IsEmpty())
	{
		if (const ULetheAttributeSet* LetheAttributeSet = AbilitySystemReferences[0].AttributeSet)
		{
			OnHealthChangedDelegate.Broadcast(LetheAttributeSet->GetHealth());
			OnMaxHealthChangedDelegate.Broadcast(LetheAttributeSet->GetMaxHealth());
		}
	}
}

void UAttributeWidgetController::OnHealthChanged(const FOnAttributeChangeData& Data) const
{
	OnHealthChangedDelegate.Broadcast(Data.NewValue);
}

void UAttributeWidgetController::OnMaxHealthChanged(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChangedDelegate.Broadcast(Data.NewValue);
}

void UAttributeWidgetController::OnCancelCardSelect()
{
	for (const auto& Elem : OnPreviewDataDelegateMap)
	{
		// Widget이 카드 선택이 취소됐음을 알 수 있도록 INDEX_NONE으로 Broadcast합니다.
		Elem.Value.Broadcast(INDEX_NONE);
	}
}
