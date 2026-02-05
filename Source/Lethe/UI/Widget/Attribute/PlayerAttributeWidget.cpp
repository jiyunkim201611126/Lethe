// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeWidget.h"

#include "PlayerAttributeWidgetController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Core/LetheTextBlock.h"
#include "Lethe/UI/Widget/ProgressBar/LetheProgressBar.h"

void UPlayerAttributeWidget::WidgetControllerSet_Implementation()
{
	if (UPlayerAttributeWidgetController* AttributeWidgetController = Cast<UPlayerAttributeWidgetController>(WidgetController))
	{
		AttributeWidgetController->OnHealthChangedDelegate.AddUObject(this, &ThisClass::OnHealthChanged);
		AttributeWidgetController->OnMaxHealthChangedDelegate.AddUObject(this, &ThisClass::OnMaxHealthChanged);
		AttributeWidgetController->OnManaChangedDelegate.AddUObject(this, &ThisClass::OnManaChanged);
		AttributeWidgetController->OnMaxManaChangedDelegate.AddUObject(this, &ThisClass::OnMaxManaChanged);
		AttributeWidgetController->OnCostChangedDelegate.AddUObject(this, &ThisClass::OnCostChanged);
		AttributeWidgetController->BroadcastInitialValue();

		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		AttributeWidgetController->OnPreviewDataDelegateMap.FindOrAdd(LetheGameplayTags.Attributes_Vital_Cost).AddUObject(this, &ThisClass::OnCostPreviewActivate);
	}
}

void UPlayerAttributeWidget::OnManaChanged(const float NewValue)
{
	Mana = NewValue;
	ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Mana, MaxMana));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Mana), FMath::RoundToInt(MaxMana)));
}

void UPlayerAttributeWidget::OnMaxManaChanged(const float NewValue)
{
	MaxMana = NewValue;
	ManaBar->SetBarPercent(UKismetMathLibrary::SafeDivide(Mana, MaxMana));
	ManaText->SetText(FText::Format(INVTEXT("{0} / {1}"), FMath::RoundToInt(Mana), FMath::RoundToInt(MaxMana)));
}

void UPlayerAttributeWidget::OnCostChanged(const float NewValue)
{
	Cost = NewValue;
	CardCostText->SetText(FText::Format(INVTEXT("{0}"), NewValue));
}

void UPlayerAttributeWidget::OnCostPreviewActivate(const float DeltaValue)
{
	const bool bPreviewEnded = FMath::IsNearlyEqual(DeltaValue, static_cast<float>(INDEX_NONE), 0.001f);
	if (bPreviewEnded)
	{
		StopAnimation(CostBlinkingAnimation);
		CardCostText->SetText(FText::Format(INVTEXT("{0}"), Cost));
	}
	else
	{
		PlayAnimation(CostBlinkingAnimation, 0, 0);
		CardCostText->SetText(FText::Format(INVTEXT("{0}"), Cost + DeltaValue));
	}
}
