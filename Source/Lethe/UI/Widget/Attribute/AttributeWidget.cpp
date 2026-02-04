// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeWidget.h"

#include "AttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UAttributeWidget::WidgetControllerSet_Implementation()
{
	if (UAttributeWidgetController* AttributeWidgetController = Cast<UAttributeWidgetController>(WidgetController))
	{
		AttributeWidgetController->OnHealthChangedDelegate.AddDynamic(this, &ThisClass::OnHealthChanged);
		AttributeWidgetController->BroadcastInitialValue();
	}
}

void UAttributeWidget::OnHealthChanged(const float Health, const float MaxHealth)
{
	HealthBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Health, MaxHealth));
	HealthText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Health), FMath::RoundToInt(MaxHealth)));
}

void UAttributeWidget::OnHealthPreviewValueChanged(const float PreviewValue)
{
	PlayAnimation(HealthBarPreviewAnimation, 0, 0);
	HealthBar->SetPreviewValue(PreviewValue);
}
