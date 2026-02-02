#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "BarWidget.generated.h"

UCLASS()
class LETHE_API UBarWidget : public UUserWidget
{
	GENERATED_BODY()


protected:
	int maxValue = 1;
	int currentValue = 0;
	
public:
	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
		UProgressBar* bar;

	UPROPERTY(EditDefaultsOnly, meta = (BindWidget))
		UTextBlock* text;

	void SetMaxValue(int value);
	void ChangeCurrentValue(int value);
	void RefreshBarPercent();


	int GetMaxValue();
	int GetCurrentValue();

};
