// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidget.h"

#include "AttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UAttributeWidget::WidgetControllerSet_Implementation()
{
	if (UAttributeWidgetController* AttributeWidgetController = Cast<UAttributeWidgetController>(WidgetController))
	{
		AttributeWidgetController->OnHealthChangedDelegate.AddUObject(this, &ThisClass::OnHealthChanged);
		AttributeWidgetController->OnMaxHealthChangedDelegate.AddUObject(this, &ThisClass::OnMaxHealthChanged);
		AttributeWidgetController->BroadcastInitialValue();
		
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		AttributeWidgetController->OnPreviewDataDelegateMap.FindOrAdd(LetheGameplayTags.Attributes_Vital_Health).AddUObject(this, &ThisClass::OnHealthPreviewValueChanged);
	}
}

void UAttributeWidget::OnHealthChanged(const float NewValue)
{
	Health = NewValue;
	HealthBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

void UAttributeWidget::OnMaxHealthChanged(const float NewValue)
{
	MaxHealth = NewValue;
	HealthBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

void UAttributeWidget::OnHealthPreviewValueChanged(const float DeltaValue) const
{
	HealthBar->SetPreviewValue(DeltaValue);
}
