// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileModeTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void FTileModeTargetTileSelector::GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<FTargetTileResult>& OutTargetResults) const
{
	OutSelectCandidateTiles.Reset();
	OutTargetResults.Reset();
	
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
		GetTargetCandidateTiles(AvatarActor, PlayerController, OutTargetResults);
	}
}

void FTileModeTargetTileSelector::GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<FTargetTileResult>& OutResults) const
{
	OutResults.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}
	
	const ICombatInterface* InstigatorCombatInterface = Cast<ICombatInterface>(AvatarActor);
	const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!InstigatorCombatInterface || !TileManagerSubsystem)
	{
		return;
	}
	
	GetTargetCandidateTiles(AvatarActor, PlayerController, OutResults);
	if (OutResults.IsEmpty())
	{
		return;
	}

	// 조건에 맞지 않는 TargetTile도 들어있으므로 걸러줘야 합니다. 일단 모두 꺼내옵니다.
	TArray<TWeakObjectPtr<ATile>> TargetCandidateTiles = MoveTemp(OutResults[0].TargetTiles);

	const ETeamSide InstigatorTeamSide = InstigatorCombatInterface->GetTeamSide();
	for (const auto& Tile : TargetCandidateTiles)
	{
		const ICombatInterface* TargetCombatInterface = Cast<ICombatInterface>(TileManagerSubsystem->GetActorOnTile(Tile.Get()));
		if (!TargetCombatInterface)
		{
			// EffectTargetMappingPolicies에서 TargetActors의 인덱스를 기반으로 로직을 수행하기 때문에, nullptr도 추가해야 합니다.
			OutResults[0].TargetTiles.Add(nullptr);
			continue;
		}

		const ETeamSide TargetTeamSide = TargetCombatInterface->GetTeamSide();
		switch (TargetTeamRelation)
		{
		case ETargetTeamRelation::AllSides:
			OutResults[0].TargetTiles.Add(Tile);
			break;
		case ETargetTeamRelation::SameTeam:
			if (InstigatorTeamSide == TargetTeamSide)
			{
				OutResults[0].TargetTiles.Add(Tile);
			}
			break;
		case ETargetTeamRelation::OpposingTeam:
			if (InstigatorTeamSide != TargetTeamSide)
			{
				OutResults[0].TargetTiles.Add(Tile);
			}
			break;
		default:
			OutResults[0].TargetTiles.Add(nullptr);
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

void FTileModeTargetTileSelector::GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<FTargetTileResult>& OutResults) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	// 마우스 커서를 기준으로 라인트레이스해서 검출된 타일을 가져옵니다.
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

	// 검출된 타일의 층수를 가져옵니다.
	const int32 HitTileFloor = TileManagerSubsystem->GetTileFloor(HitTile);

	// 조건에 맞는 타일들을 모두 가져옵니다.
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

	// '타겟 후보'를 찾는 중이므로, 조건을 따지지 않고 검출된 모든 타일을 Out 인자에 넣어줍니다.
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FTargetTileResult& PrimaryTargets = OutResults.Emplace_GetRef();
	PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Primary;
	PrimaryTargets.TargetTiles.Reserve(OutCubeCoords.Num());
	
	for (const FCubeCoord& CubeCoord : OutCubeCoords)
	{
		PrimaryTargets.TargetTiles.Add(TileManagerSubsystem->GetTile(CubeCoord));
	}
}
