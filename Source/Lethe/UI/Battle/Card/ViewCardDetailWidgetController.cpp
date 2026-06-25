// Copyright JETBLU, Inc. All Rights Reserved.

#include "ViewCardDetailWidgetController.h"

#include "Lethe/Controller/PlayerController/LethePlayerController.h"

void UViewCardDetailWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	Super::SetWidgetControllerParams(WidgetControllerParams);

	LethePlayerController = CastChecked<ALethePlayerController>(PlayerController);
}

void UViewCardDetailWidgetController::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const
{
	if (LethePlayerController)
	{
		LethePlayerController->GetCardDescriptionText(OwnerASC, SavedCard, OutText);
	}
}
