// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidget.h"

#include "AttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UPlayerAttributeWidget::WidgetControllerSet_Implementation()
{
	if (UAttributeWidgetController* AttributeWidgetController = Cast<UAttributeWidgetController>(WidgetController))
	{
		AttributeWidgetController->OnHealthPercentChangedDelegate.AddDynamic(this, &ThisClass::OnHealthChanged);
		AttributeWidgetController->OnManaPercentChangedDelegate.AddDynamic(this, &ThisClass::OnManaChanged);
		AttributeWidgetController->OnCostChangedDelegate.AddDynamic(this, &ThisClass::OnCostChanged);
		AttributeWidgetController->BroadcastInitialValue();
	}
}

void UPlayerAttributeWidget::OnManaChanged(const float Mana, const float MaxMana)
{
	ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Mana, MaxMana));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Mana), FMath::RoundToInt(MaxMana)));
}

void UPlayerAttributeWidget::OnCostChanged(const float Cost)
{
	CostText->SetText(FText::Format(INVTEXT("{0}"), Cost));
}
