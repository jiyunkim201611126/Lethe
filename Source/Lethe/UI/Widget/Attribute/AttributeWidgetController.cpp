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

	Health = AS->GetHealth();
	MaxHealth = AS->GetMaxHealth();
}

void UAttributeWidgetController::BroadcastInitialValue()
{
	OnHealthChangedDelegate.Broadcast(Health, MaxHealth);
}

void UAttributeWidgetController::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	Health = Data.NewValue;
	OnHealthChangedDelegate.Broadcast(Health, MaxHealth);
}

void UAttributeWidgetController::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	MaxHealth = Data.NewValue;
	OnHealthChangedDelegate.Broadcast(Health, MaxHealth);
}
