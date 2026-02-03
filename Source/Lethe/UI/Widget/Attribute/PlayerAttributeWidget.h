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
	void OnManaChanged(const float Mana, const float MaxMana);

	UFUNCTION()
	void OnCostChanged(const float Cost);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> ManaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> ManaText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> CostText;
};
