#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BarWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "StatInfoWidget.generated.h"

UCLASS()

class LETHE_API UStatInfoWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BindWidget))
		UBarWidget* chaHPBar;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BindWidget))
		UBarWidget* chaMPBar;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BindWidget))
		UTextBlock* chaCost;
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (BindWidget))
		UImage* costImage;

public:
	void SettingValues(int maxHP, int currentHP, int maxMP, int currentMP, int currentCost);
	void SettingValuesMon(int maxHP, int currentHP);
	void UpdateHPBar(int value);
	void UpdateMPBar(int value);
	void UpdateCost(int value);
};
