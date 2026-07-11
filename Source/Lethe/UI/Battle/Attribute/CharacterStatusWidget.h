// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "CharacterStatusWidget.generated.h"

class ULetheTextBlock;
class ULetheProgressBar;
struct FAttributeData;

UCLASS(Abstract)
class LETHE_API UCharacterStatusWidget : public ULetheUserWidget
{
	GENERATED_BODY()

protected:
	//~ Begin ULetheUserWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheUserWidget Interface
	
private:
	void UpdateHealthUI(const FAttributeData& NewData) const;
	void StartPreviewHealth(const FAttributeData& NewData) const;
	void StopPreviewHealth(const FAttributeData& NewData) const;

protected:
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	TObjectPtr<ULetheProgressBar> HealthBar;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<ULetheTextBlock> HealthText;
};
