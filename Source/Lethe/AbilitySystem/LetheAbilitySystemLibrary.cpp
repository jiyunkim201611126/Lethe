// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/UI/HUD/LetheHUD.h"

UOverlayWidgetController* ULetheAbilitySystemLibrary::GetOverlayWidgetController(const UObject* WorldContextObject)
{
	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALetheHUD* LetheHUD = PlayerController->GetHUD<ALetheHUD>())
		{
			return LetheHUD->GetOverlayWidgetController();
		}
	}
	return nullptr;
}

UCardPanelWidgetController* ULetheAbilitySystemLibrary::GetCardPanelWidgetController(const UObject* WorldContextObject)
{
	if (const APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
	{
		if (const ALetheHUD* LetheHUD = PlayerController->GetHUD<ALetheHUD>())
		{
			return LetheHUD->GetCardPanelWidgetController();
		}
	}
	return nullptr;
}
