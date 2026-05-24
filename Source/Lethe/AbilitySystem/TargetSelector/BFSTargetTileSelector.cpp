// Copyright JETBLU, Inc. All Rights Reserved.

#include "BFSTargetTileSelector.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void UBFSTargetTileSelector::Select(const AActor* AvatarActor, const ATile* CenterTile, TArray<AActor*>& OutTiles)
{
	Super::Select(AvatarActor, CenterTile, OutTiles);
	
	if (!AvatarActor || !CenterTile)
	{
		return;
	}
	
	if (const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		const int32 CenterTileFloor = TileManagerSubsystem->GetTileFloor(CenterTile);
		
		TSet<FCubeCoord> OutCubeCoords;
		TileManagerSubsystem->TileBFS(CenterTile->GetCubeCoord(), Range.Distance, Range.BFSType, OutCubeCoords,
			[](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				return true;
			},
			[this, TileManagerSubsystem, CenterTileFloor](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
			{
				if (TileData && TileData->TopTile.IsValid())
				{
					const ATile* Tile = TileData->TopTile.Get();
					const int32 CurrentTargetFloor = TileManagerSubsystem->GetTileFloor(Tile);
					const int32 FloorGap = CurrentTargetFloor - CenterTileFloor;

					if (FloorGap <= Range.FloorGap)
					{
						return true;
					}
				}
				return false;
			});

		for (const FCubeCoord& CubeCoord : OutCubeCoords)
		{
			OutTiles.Add(TileManagerSubsystem->GetTile(CubeCoord));
		}
	}
}
