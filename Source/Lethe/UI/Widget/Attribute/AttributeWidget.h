// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheUserWidget.h"
#include "AttributeWidget.generated.h"

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

protected:
	UFUNCTION()
	void OnHealthChanged(const float NewValue);
	
	UFUNCTION()
	void OnMaxHealthChanged(const float NewValue);
	
	void OnHealthPreviewValueChanged(const float DeltaValue) const;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> HealthText;

private:
	float Health = 0.f;
	float MaxHealth = 0.f;
};
