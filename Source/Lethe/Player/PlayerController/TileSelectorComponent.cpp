// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileSelectorComponent.h"

#include "Lethe/Lethe.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

UTileSelectorComponent::UTileSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTileSelectorComponent::HighlightTileByMouse(AActor* Tile)
{
	if (Tile)
	{
		LastMouseHoveredTile = CurrentMouseHoveredTile;
		CurrentMouseHoveredTile = Tile;

		if (LastMouseHoveredTile != CurrentMouseHoveredTile)
		{
			if (LastMouseHoveredTile)
			{
				IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredTile.GetObject());
			}
			if (CurrentMouseHoveredTile)
			{
				IHighlightInterface::Execute_HighlightActorByMouse(CurrentMouseHoveredTile.GetObject());
			}
		}
	}
}

void UTileSelectorComponent::UnhighlightTileByMouse()
{
	if (LastMouseHoveredTile)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredTile.GetObject());
		LastMouseHoveredTile = nullptr;
	}
	if (CurrentMouseHoveredTile)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(CurrentMouseHoveredTile.GetObject());
		CurrentMouseHoveredTile = nullptr;
	}
}

AActor* UTileSelectorComponent::GetActorOnTileUnderCursor() const
{
	if (const APlayerController* PlayerController = GetOwner<APlayerController>())
	{
		FHitResult Hit;
		PlayerController->GetHitResultUnderCursor(ECC_Tile, false, Hit);

		if (Hit.IsValidBlockingHit())
		{
			if (ATile* HitTile = Cast<ATile>(Hit.GetActor()))
			{
				const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
				const ATile* TopTile = HitTile->GetTopTile();
				if (TileManagerSubsystem && TopTile)
				{
					return TileManagerSubsystem->GetActorOnTile(TopTile);
				}
			}
		}
	}

	return nullptr;
}

bool UTileSelectorComponent::TryGetTilesByDepth(TArray<ATile*>& OutTiles, const FCubeCoord& CenterCoord, const int32 InDepth) const
{
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		TSet<FCubeCoord> SelectedCoords;
		TileManagerSubsystem->TileBFS(SelectedCoords, CenterCoord, InDepth, EBFSType::Connection,
			[]()
			{
				return true;
			},
			[](const FTileData* TileData, int32 Depth)
			{
				return true;
			});

		for (const FCubeCoord& SelectedCoord : SelectedCoords)
		{
			if (ATile* SelectedTile = TileManagerSubsystem->GetTile(SelectedCoord))
			{
				OutTiles.Emplace(SelectedTile);
			}
		}
	}

	return OutTiles.Num() > 0;
}

