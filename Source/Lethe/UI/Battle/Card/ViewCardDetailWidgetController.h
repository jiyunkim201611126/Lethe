// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheWidgetController.h"
#include "ViewCardDetailWidgetController.generated.h"

class ALethePlayerController;
struct FSavedCard;

UCLASS()
class LETHE_API UViewCardDetailWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin ULetheWidgetController Interface
	virtual void SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams) override;
	//~ End of ULetheWidgetController Interface
	
	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const;

private:
	UPROPERTY()
	TObjectPtr<ALethePlayerController> LethePlayerController;
};
