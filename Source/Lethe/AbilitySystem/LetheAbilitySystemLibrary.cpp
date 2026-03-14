// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Manager/TileManagerSubsystem.h"
#include "Lethe/UI/HUD/LetheHUD.h"

class UTileManagerSubsystem;

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

bool ULetheAbilitySystemLibrary::CanUseAbilityByActorAndFloorGap(const UObject* WorldContextObject, const AActor* SourceActor, const AActor* TargetActor, const int32 MaxFloorGap)
{
	if (WorldContextObject && SourceActor && TargetActor)
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = WorldContextObject->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			const ATile* SourceTile = TileManagerSubsystem->GetTileUnderActor(SourceActor);
			const ATile* TargetTile = TileManagerSubsystem->GetTileUnderActor(TargetActor);
			if (SourceTile && TargetTile)
			{
				return CanUseAbilityByTileAndFloorGap(WorldContextObject, SourceTile, TargetTile, MaxFloorGap);
			}
		}
	}
	return false;
}

bool ULetheAbilitySystemLibrary::CanUseAbilityByTileAndFloorGap(const UObject* WorldContextObject, const ATile* SourceTile, const ATile* TargetTile, const int32 MaxFloorGap)
{
	if (WorldContextObject && SourceTile && TargetTile)
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = WorldContextObject->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			const int32 CurrentFloor = TileManagerSubsystem->GetTileFloor(SourceTile);
			const int32 TargetFloor = TileManagerSubsystem->GetTileFloor(TargetTile);
			const int32 FloorGap = CurrentFloor - TargetFloor;
			return FloorGap <= MaxFloorGap;
		}
	}
	return false;
}
