// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileSelectorComponent.h"

#include "Lethe/Lethe.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/HighlightInterface.h"

UTileSelectorComponent::UTileSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTileSelectorComponent::HighlightTile(AActor* Tile)
{
	if (Tile)
	{
		LastHoveredTile = CurrentHoveredTile;
		CurrentHoveredTile = Tile;

		if (LastHoveredTile != CurrentHoveredTile)
		{
			if (LastHoveredTile)
			{
				IHighlightInterface::Execute_UnhighlightActorByMouse(LastHoveredTile.GetObject());
			}
			if (CurrentHoveredTile)
			{
				IHighlightInterface::Execute_HighlightActorByMouse(CurrentHoveredTile.GetObject());
			}
		}
	}
}

void UTileSelectorComponent::UnhighlightTile()
{
	if (LastHoveredTile)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(LastHoveredTile.GetObject());
		LastHoveredTile = nullptr;
	}
	if (CurrentHoveredTile)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(CurrentHoveredTile.GetObject());
		CurrentHoveredTile = nullptr;
	}
}

AActor* UTileSelectorComponent::TryGetActorOnTileUnderCursor() const
{
	if (const APlayerController* PlayerController = GetOwner<APlayerController>())
	{
		FHitResult Hit;
		PlayerController->GetHitResultUnderCursor(ECC_Tile, false, Hit);

		if (Hit.IsValidBlockingHit())
		{
			if (const ATile* Tile = Cast<ATile>(Hit.GetActor()))
			{
				return Tile->GetActorOnTile<AActor>();
			}
		}
	}

	return nullptr;
}

