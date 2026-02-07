// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "AttributeWidget.generated.h"

struct FAttributeData;
class ULetheTextBlock;
class ULetheProgressBar;

UCLASS()
class LETHE_API UAttributeWidget : public ULetheUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface
	
private:
	void UpdateHealthUI(const FAttributeData& NewData) const;
	void StopPreviewHealth(const FAttributeData& NewData) const;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> HealthText;
};
