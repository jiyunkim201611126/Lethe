// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeWidget.h"
#include "PlayerAttributeWidget.generated.h"

UCLASS()
class LETHE_API UPlayerAttributeWidget : public UAttributeWidget
{
	GENERATED_BODY()

protected:
	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface

private:
	UFUNCTION()
	void OnManaChanged(const float NewValue);
	
	UFUNCTION()
	void OnMaxManaChanged(const float NewValue);

	UFUNCTION()
	void OnCostChanged(const float NewValue);

	UFUNCTION()
	void OnCostPreviewActivate(const float DeltaValue);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> ManaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> ManaText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> CardCostText;

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	TObjectPtr<UWidgetAnimation> CostBlinkingAnimation;

private:
	float Mana = 0.f;
	float MaxMana = 0.f;
	float Cost = 0.f;
};
