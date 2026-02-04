// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidget.h"

#include "PlayerAttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UPlayerAttributeWidget::WidgetControllerSet_Implementation()
{
	if (UPlayerAttributeWidgetController* AttributeWidgetController = Cast<UPlayerAttributeWidgetController>(WidgetController))
	{
		AttributeWidgetController->OnHealthChangedDelegate.AddDynamic(this, &ThisClass::OnHealthChanged);
		AttributeWidgetController->OnManaChangedDelegate.AddDynamic(this, &ThisClass::OnManaChanged);
		AttributeWidgetController->OnCostChangedDelegate.AddDynamic(this, &ThisClass::OnCostChanged);

		AttributeWidgetController->OnPreviewDataChangedMap.(this, &ThisClass::OnAbilityCostPreviewValueChanged);
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
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), Cost));
}

void UPlayerAttributeWidget::OnAbilityCostPreviewValueChanged(const float PreviewValue)
{
	PlayAnimation(CostPreviewAnimation, 0, 0);
}
