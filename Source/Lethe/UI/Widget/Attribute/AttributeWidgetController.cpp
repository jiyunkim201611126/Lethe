// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

void UAttributeWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	// Attribute들에게 변동사항이 있는 경우 Widget Controller가 알 수 있도록 각 AttributeSet에게 함수를 바인드합니다.
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetManaAttribute()).AddUObject(this, &ThisClass::OnManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxManaAttribute()).AddUObject(this, &ThisClass::OnMaxManaChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetCostAttribute()).AddUObject(this, &ThisClass::OnCostChanged);

	Health = AS->GetHealth();
	MaxHealth = AS->GetMaxHealth();
	Mana = AS->GetMana();
	MaxMana = AS->GetMaxMana();
	Cost = AS->GetCost();
}

void UAttributeWidgetController::BindCallbacksForEnemy(ULetheAbilitySystemComponent* ASC, const ULetheAttributeSet* AS)
{
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	ASC->GetGameplayAttributeValueChangeDelegate(AS->GetMaxHealthAttribute()).AddUObject(this, &ThisClass::OnMaxHealthChanged);

	Health = AS->GetHealth();
	MaxHealth = AS->GetMaxHealth();
}

void UAttributeWidgetController::BroadcastInitialValue()
{
	OnHealthPercentChangedDelegate.Broadcast(Health, MaxHealth);
	OnManaPercentChangedDelegate.Broadcast(Mana, MaxMana);
	OnCostChangedDelegate.Broadcast(Cost);
}

void UAttributeWidgetController::OnHealthChanged(const FOnAttributeChangeData& Data)
{
	Health = Data.NewValue;
	OnHealthPercentChangedDelegate.Broadcast(Health, MaxHealth);
}

void UAttributeWidgetController::OnMaxHealthChanged(const FOnAttributeChangeData& Data)
{
	MaxHealth = Data.NewValue;
	OnHealthPercentChangedDelegate.Broadcast(Health, MaxHealth);
}

void UAttributeWidgetController::OnManaChanged(const FOnAttributeChangeData& Data)
{
	Mana = Data.NewValue;
	OnManaPercentChangedDelegate.Broadcast(Mana, MaxMana);
}

void UAttributeWidgetController::OnMaxManaChanged(const FOnAttributeChangeData& Data)
{
	MaxMana = Data.NewValue;
	OnManaPercentChangedDelegate.Broadcast(Mana, MaxMana);
}

void UAttributeWidgetController::OnCostChanged(const FOnAttributeChangeData& Data)
{
	Cost = Data.NewValue;
	OnCostChangedDelegate.Broadcast(Cost);
}
