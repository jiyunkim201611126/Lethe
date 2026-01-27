// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/Player/LethePlayerController.h"
#include "Lethe/UI/HUD/LetheHUD.h"

UOverlayWidgetController* ULetheAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
		{
			if (const ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetOverlayWidgetController();
			}
		}
	}
	return nullptr;
}

UCardPanelWidgetController* ULetheAbilitySystemLibrary::GetCardPanelWidgetController(const UObject* WorldContextObject)
{
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
		{
			if (const ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				return LetheHUD->GetCardPanelWidgetController();
			}
		}
	}
	return nullptr;
}
