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

	UFUNCTION(BlueprintImplementableEvent)
	void PlayPreviewCostAnimation(const float InTime, const float PreviewCostValue);

	UFUNCTION(BlueprintImplementableEvent)
	void StopPreviewCostAnimation();

private:
	void UpdateManaUI(const FAttributeData& NewData) const;
	void UpdateCostUI(const FAttributeData& NewData);
	void StartPreviewMana(const FAttributeData& NewData) const;
	void StartPreviewCost(const FAttributeData& NewData);
	void StopPreviewMana(const FAttributeData& NewData) const;
	void StopPreviewCost(const FAttributeData& NewData);

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> ManaBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> ManaText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> CardCostText;
};
