// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileModeTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void FTileModeTargetTileSelector::GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	if (!Context.IsValid())
	{
		return;
	}
	
	GetSelectCandidateTiles(Context);

	if (Context.OutSelectCandidateTiles.Contains(Context.TargetingIntent.HitTile))
	{
		// 마우스를 올린 타일이 선택 후보 타일에 포함되는 경우에만 타겟 후보 타일을 Out 인자에 채워줍니다.
		GetTargetCandidateTiles(Context);
	}
}

void FTileModeTargetTileSelector::GetTargetTiles(FEffectTargetTileSelectorContext& Context) const
{
	if (!Context.IsValid())
	{
		return;
	}
	
	GetTargetCandidateTiles(Context);
	ResolveTargets(Context);
}

void FTileModeTargetTileSelector::GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	const int32 SourceTileFloor = Context.TileManagerSubsystem->GetTileFloor(Context.SourceTile);
	
	TSet<FCubeCoord> OutCubeCoords;
	Context.TileManagerSubsystem->TileBFS(Context.SourceTile->GetCubeCoord(), SelectRange.Distance, SelectRange.BFSType, OutCubeCoords,
		[](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return true;
		},
		[this, Context, SourceTileFloor](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			if (TileData && TileData->TopTile.IsValid())
			{
				const int32 CurrentTargetFloor = Context.TileManagerSubsystem->GetTileFloor(TileData->TopTile.Get());
				const int32 FloorGap = CurrentTargetFloor - SourceTileFloor;

				if (FloorGap <= SelectRange.FloorGap)
				{
					return true;
				}
			}
			return false;
		});

	for (const FCubeCoord& CubeCoord : OutCubeCoords)
	{
		if (ATile* SelectCandidateTile = Context.TileManagerSubsystem->GetTile(CubeCoord))
		{
			Context.OutSelectCandidateTiles.Add(SelectCandidateTile);
		}
	}
}

void FTileModeTargetTileSelector::GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	// 검출된 타일의 층수를 가져옵니다.
	const int32 HitTileFloor = Context.TileManagerSubsystem->GetTileFloor(Context.TargetingIntent.HitTile);

	// 조건에 맞는 타일들을 모두 가져옵니다.
	TSet<FCubeCoord> OutCubeCoords;
	Context.TileManagerSubsystem->TileBFS(Context.TargetingIntent.HitTile->GetCubeCoord(), TargetTileRange.Distance, TargetTileRange.BFSType, OutCubeCoords,
		[](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return true;
		},
		[this, Context, HitTileFloor](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			if (TileData && TileData->TopTile.IsValid())
			{
				const int32 CurrentTargetFloor = Context.TileManagerSubsystem->GetTileFloor(TileData->TopTile.Get());
				const int32 FloorGap = CurrentTargetFloor - HitTileFloor;

				if (FloorGap <= TargetTileRange.FloorGap)
				{
					return true;
				}
			}
			return false;
		});

	// '타겟 후보'를 찾는 중이므로, 조건을 따지지 않고 검출된 모든 타일을 Out 인자에 넣어줍니다.
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FTargetSelectionResult& PrimaryTargets = Context.OutTargetCandidates.AddDefaulted_GetRef();
	PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetGroup_Primary;
	PrimaryTargets.Targets.Reserve(OutCubeCoords.Num());
	
	for (const FCubeCoord& CubeCoord : OutCubeCoords)
	{
		if (ATile* TargetTile = Context.TileManagerSubsystem->GetTile(CubeCoord))
		{
			FSelectedTarget& Target = PrimaryTargets.Targets.AddDefaulted_GetRef();
			Target.TargetTile = TargetTile;
		}
	}
}
