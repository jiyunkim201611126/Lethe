// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileModeTargetTileSelector.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void FTileModeTargetTileSelector::GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles) const
{
	OutSelectCandidateTiles.Reset();
	OutTargetCandidateTiles.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}
	
	GetSelectCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles);
	
	FHitResult HitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult);
	if (!HitResult.IsValidBlockingHit())
	{
		return;
	}

	const ATile* HitTile = Cast<ATile>(HitResult.GetActor());
	if (!HitTile)
	{
		return;
	}

	if (OutSelectCandidateTiles.Contains(HitTile))
	{
		// 마우스를 올린 타일이 선택 후보 타일에 포함되는 경우에만 타겟 후보 타일을 Out 인자에 채워줍니다.
		GetTargetCandidateTiles(AvatarActor, PlayerController, OutTargetCandidateTiles);
	}
}

void FTileModeTargetTileSelector::GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}
	
	TArray<ATile*> OutCandidateTargetTiles;
	GetTargetCandidateTiles(AvatarActor, PlayerController, OutCandidateTargetTiles);
	
	const ICombatInterface* InstigatorCombatInterface = Cast<ICombatInterface>(AvatarActor);
	if (!InstigatorCombatInterface)
	{
		return;
	}
	
	const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	const ETeamSide InstigatorTeamSide = InstigatorCombatInterface->GetTeamSide();
	for (ATile* Tile : OutCandidateTargetTiles)
	{
		const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(TileManagerSubsystem->GetActorOnTile(Tile));
		if (!TargetCombatInterface)
		{
			// EffectTargetMappingPolicies에서 TargetActors의 인덱스를 기반으로 로직을 수행하기 때문에, nullptr도 추가해야 합니다.
			OutTiles.Add(nullptr);
			continue;
		}

		const ETeamSide TargetTeamSide = TargetCombatInterface->GetTeamSide();
		switch (TargetTeamRelation)
		{
		case ETargetTeamRelation::AllSides:
			OutTiles.Add(Tile);
			break;
		case ETargetTeamRelation::SameTeam:
			if (InstigatorTeamSide == TargetTeamSide)
			{
				OutTiles.Add(Tile);
			}
			break;
		case ETargetTeamRelation::OpposingTeam:
			if (InstigatorTeamSide != TargetTeamSide)
			{
				OutTiles.Add(Tile);
			}
			break;
		default:
			OutTiles.Add(nullptr);
			break;
		}
	}
}

void FTileModeTargetTileSelector::GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	const ATile* CurrentTile = TileManagerSubsystem->GetTileUnderActor(AvatarActor);
	if (!CurrentTile)
	{
		return;
	}
	
	const int32 CurrentTileFloor = TileManagerSubsystem->GetTileFloor(CurrentTile);
	
	TSet<FCubeCoord> OutCubeCoords;
	TileManagerSubsystem->TileBFS(CurrentTile->GetCubeCoord(), SelectRange.Distance, SelectRange.BFSType, OutCubeCoords,
		[](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return true;
		},
		[this, TileManagerSubsystem, CurrentTileFloor](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			if (TileData && TileData->TopTile.IsValid())
			{
				const ATile* Tile = TileData->TopTile.Get();
				const int32 CurrentTargetFloor = TileManagerSubsystem->GetTileFloor(Tile);
				const int32 FloorGap = CurrentTargetFloor - CurrentTileFloor;

				if (FloorGap <= SelectRange.FloorGap)
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

void FTileModeTargetTileSelector::GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	FHitResult HitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult);
	if (!HitResult.IsValidBlockingHit())
	{
		return;
	}

	const ATile* HitTile = Cast<ATile>(HitResult.GetActor());
	if (!HitTile)
	{
		return;
	}
	
	const int32 HitTileFloor = TileManagerSubsystem->GetTileFloor(HitTile);
	
	TSet<FCubeCoord> OutCubeCoords;
	TileManagerSubsystem->TileBFS(HitTile->GetCubeCoord(), TargetTileRange.Distance, TargetTileRange.BFSType, OutCubeCoords,
		[](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			return true;
		},
		[this, TileManagerSubsystem, HitTileFloor](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			if (TileData && TileData->TopTile.IsValid())
			{
				const ATile* Tile = TileData->TopTile.Get();
				const int32 CurrentTargetFloor = TileManagerSubsystem->GetTileFloor(Tile);
				const int32 FloorGap = CurrentTargetFloor - HitTileFloor;

				if (FloorGap <= TargetTileRange.FloorGap)
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
