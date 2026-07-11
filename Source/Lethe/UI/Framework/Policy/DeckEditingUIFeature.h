// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameUIFeature.h"
#include "DeckEditingUIFeature.generated.h"

class UDeckEditingWidget;

UCLASS()
class LETHE_API UDeckEditingUIFeature : public ULetheGameUIFeature
{
	GENERATED_BODY()

public:
	virtual void InitializeFeature(ULethePrimaryGameLayout* InLayoutWidget) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UDeckEditingWidget> DeckEditingWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UDeckEditingWidget> DeckEditingWidget;
};
