#include "BarWidget.h"


void UBarWidget::SetMaxValue(int value)
{
	maxValue = value;
}

void UBarWidget::ChangeCurrentValue(int value)
{
	currentValue = value;
	RefreshBarPercent();
}

void UBarWidget::RefreshBarPercent()
{
	float percent = (float)currentValue / (float)maxValue;

	bar->SetPercent(percent);
	text->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), currentValue, maxValue)));
}

int UBarWidget::GetMaxValue()
{
	return maxValue;
}

int UBarWidget::GetCurrentValue()
{
	return currentValue;
}

