#include "StatInfoWidget.h"

void UStatInfoWidget::SettingValues(int maxHP, int currentHP, int maxMP, int currentMP, int currentCost)
{
	chaHPBar->SetMaxValue(maxHP);
	UpdateHPBar(currentHP);
	chaMPBar->SetMaxValue(maxMP);
	UpdateMPBar(currentMP);
	UpdateCost(currentCost);
}

void UStatInfoWidget::SettingValuesMon(int maxHP, int currentHP)
{
	chaHPBar->SetMaxValue(maxHP);
	UpdateHPBar(currentHP);

	chaMPBar->SetVisibility(ESlateVisibility::Collapsed);
	chaCost->SetVisibility(ESlateVisibility::Collapsed);
	costImage->SetVisibility(ESlateVisibility::Collapsed);
}

void UStatInfoWidget::UpdateHPBar(int value)
{
	chaHPBar->ChangeCurrentValue(value);
}

void UStatInfoWidget::UpdateMPBar(int value)
{
	chaMPBar->ChangeCurrentValue(value);
}

void UStatInfoWidget::UpdateCost(int value)
{
	FString costString = FString::FromInt(value);
	FText costText = FText::FromString(costString);
	chaCost->SetText(costText);
}