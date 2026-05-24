// Copyright JETBLU, Inc. All Rights Reserved.

#include "BFSTargetSelector.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void UBFSTargetSelector::Select(const AActor* AvatarActor, const ATile* CenterTile, TArray<TWeakObjectPtr<ATile>>& OutTiles)
{
	Super::Select(AvatarActor, CenterTile, OutTiles);
	
	if (!AvatarActor || !CenterTile)
	{
		return;
	}
	
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		TSet<FCubeCoord> OutCubeCoords;
		TileManagerSubsystem->TileBFS(CenterTile->GetCubeCoord(), Range.Distance, Range.BFSType, OutCubeCoords,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[&OutTiles](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
			{
				if (TileData)
				{
					OutTiles.Add(TileData->TopTile);
				}
				return false;
			});
	}
}
