// Copyright JETBLU, Inc. All Rights Reserved.

#include "DirectionModeTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void FDirectionModeTargetTileSelector::GetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	if (!Context.IsValid())
	{
		return;
	}

	GetSelectCandidateTiles(Context);
	GetTargetCandidateTiles(Context);
}

void FDirectionModeTargetTileSelector::GetTargetTiles(FEffectTargetTileSelectorContext& Context) const
{
	if (!Context.IsValid())
	{
		return;
	}
	
	// TargetTile 계산 시 SelectCandidateTile이 필요하므로 여기서 호출합니다.
	GetSelectCandidateTiles(Context);
	GetTargetCandidateTiles(Context);
}

void FDirectionModeTargetTileSelector::GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(Context.CurrentTile, Context.PlayerController, SelectedDirections);

	const FCubeCoord CenterCoord = Context.CurrentTile->GetCubeCoord();
	const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);

	// 지정된 방향을 모두 순회하며, 해당 방향으로 1칸씩 뻗어나가면서 모든 타일을 OutTiles에 추가합니다.
	for (const int32 Direction : SelectedDirections)
	{
		for (int32 Distance = 1; Distance <= MaxRangeDistance; ++Distance)
		{
			const FCubeCoord DirectionOffset = FCubeCoord::GetDirection(Direction);
			const FCubeCoord TargetCoord(
				CenterCoord.Q + DirectionOffset.Q * Distance,
				CenterCoord.R + DirectionOffset.R * Distance,
				CenterCoord.S + DirectionOffset.S * Distance);

			// 인덱스가 곧 CurrentTile과의 거리를 나타내므로, nullptr이더라도 추가합니다.
			Context.OutSelectCandidateTiles.Add(Context.TileManagerSubsystem->GetTile(TargetCoord));
		}
	}
}

void FDirectionModeTargetTileSelector::GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	TArray<FResolvedPrimaryTargetTile> OutData;
	switch (RangeType)
	{
	case ERangeType::Melee:
	case ERangeType::ParabolaRanged:
		HandleMeleeAndParabolaRanged(Context, OutData);
		break;
	case ERangeType::StraightRanged:
		HandleStraightRanged(Context, OutData);
		break;
	}
	HandleAdditionalRanges(Context, OutData);
}

void FDirectionModeTargetTileSelector::HandleMeleeAndParabolaRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const
{
	FHitResult HitResult;
	if (Context.PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult))
	{
		// AvatarActor가 서있는 타일과 마우스 위치까지의 거리를 계산합니다.
		const FVector CurrentTileLocation = Context.CurrentTile->GetActorLocation();
		const FVector HitLocation = HitResult.ImpactPoint;
		const float HitDistance = FVector::Dist(CurrentTileLocation, HitLocation);

		// 타일과 타일 사이의 거리를 가져옵니다.
		const float TileWidthInterval = FCubeCoord::GetTileWidthInterval();

		// 거리를 타일 기준으로 계산합니다.
		const int32 HitTileDistance = FMath::RoundToInt(HitDistance / TileWidthInterval);

		const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);
		if (!(0 <= HitTileDistance && HitTileDistance <= MaxRangeDistance))
		{
			// 사거리를 벗어나 마우스를 둔 경우 얼리리턴합니다.
			return;
		}
		
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		FTargetTileResult& PrimaryTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
		PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Primary;

		// AdditionalRange 계산을 위해 Direction이 필요하므로, 여기서 다시 가져옵니다.
		TArray<int32> SelectedDirections;
		GetSelectedDirections(Context.CurrentTile, Context.PlayerController, SelectedDirections);

		// 거리에 알맞는 타일들만 추가합니다.
		int32 TileIndex = HitTileDistance - 1;
		while (Context.OutSelectCandidateTiles.IsValidIndex(TileIndex))
		{
			const int32 DirectionIndex = TileIndex / MaxRangeDistance;
			if (!SelectedDirections.IsValidIndex(DirectionIndex))
			{
				break;
			}

			const int32 Direction = SelectedDirections[DirectionIndex];

			ATile* TargetTile = Context.OutSelectCandidateTiles[TileIndex];
			PrimaryTargets.TargetTiles.Add(TargetTile);

			FResolvedPrimaryTargetTile& AddedData = OutTargetTiles.AddDefaulted_GetRef();
			AddedData.Tile = TargetTile;
			AddedData.Direction = Direction;
			AddedData.Distance = HitTileDistance;
			
			TileIndex += MaxRangeDistance;
		}
	}
}

void FDirectionModeTargetTileSelector::HandleStraightRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const
{
	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(Context.CurrentTile, Context.PlayerController, SelectedDirections);

	const FCubeCoord CenterCoord = Context.CurrentTile->GetCubeCoord();
	const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);
		
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FTargetTileResult& PrimaryTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
	PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Primary;
	PrimaryTargets.TargetTiles.Reserve(SelectedDirections.Num() * MaxRangeDistance);

	// 지정된 방향을 모두 순회하며, 해당 방향으로 1칸씩 뻗어나갑니다.
	for (const int32 Direction : SelectedDirections)
	{
		for (int32 Distance = 1; Distance <= MaxRangeDistance; ++Distance)
		{
			const FCubeCoord DirectionOffset = FCubeCoord::GetDirection(Direction);
			const FCubeCoord TargetCoord(
				CenterCoord.Q + DirectionOffset.Q * Distance,
				CenterCoord.R + DirectionOffset.R * Distance,
				CenterCoord.S + DirectionOffset.S * Distance);

			if (ATile* Tile = Context.TileManagerSubsystem->GetTile(TargetCoord))
			{
				if (const AActor* ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(Tile))
				{
					if (ActorOnTile->Implements<UCombatInterface>())
					{
						// 전투 가능한 액터가 올라서있다면 타일을 추가합니다.
						PrimaryTargets.TargetTiles.Add(Tile);

						FResolvedPrimaryTargetTile& AddedData = OutTargetTiles.AddDefaulted_GetRef();
						AddedData.Tile = Tile;
						AddedData.Direction = Direction;
						AddedData.Distance = Distance;
					}
					else
					{
						// 전투할 수 없는 액터가 올라서있다면 nullptr을 추가합니다.
						PrimaryTargets.TargetTiles.Add(nullptr);
					}
					// 일단 액터를 만났다면 다른 방향을 탐색합니다.
					break;
				}
				// 액터를 찾지 못 했다면 이 방향을 계속해서 나아갑니다.
				continue;
			}
			
			// 액터를 마주치기 전에 맵 바깥으로 나가버렸다면, nullptr을 추가하고 다른 방향을 탐색합니다.
			PrimaryTargets.TargetTiles.Add(nullptr);
			break;
		}
	}
}

void FDirectionModeTargetTileSelector::HandleAdditionalRanges(FEffectTargetTileSelectorContext& Context, const TArray<FResolvedPrimaryTargetTile>& ResolvedTargetTiles) const
{
	if (ResolvedTargetTiles.IsEmpty())
	{
		return;
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	for (const auto& AdditionalRange : AdditionalRanges)
	{
		switch (AdditionalRange.Key)
		{
		case EAdditionalRangeType::Penetration:
			{
				FTargetTileResult& PenetrationTarget = Context.OutTargetTileResults.AddDefaulted_GetRef();
				PenetrationTarget.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Penetration;
				for (const FResolvedPrimaryTargetTile& TargetTile : ResolvedTargetTiles)
				{
					if (!TargetTile.Tile)
					{
						continue;
					}
					
					const FCubeCoord DirectionCoord = FCubeCoord::GetDirection(TargetTile.Direction);
					FCubeCoord PenetrationCoord = TargetTile.Tile->GetCubeCoord();

					for (int32 Count = 0; Count < AdditionalRange.Value; ++Count)
					{
						PenetrationCoord = PenetrationCoord + DirectionCoord;
						if (ATile* PenetrationTile = Context.TileManagerSubsystem->GetTile(PenetrationCoord))
						{
							PenetrationTarget.TargetTiles.Add(PenetrationTile);
						}
					}
				}
			}
			break;
		default:
			break;
		}
	}
}

int32 FDirectionModeTargetTileSelector::NormalizeHexDirection(const int32 Direction) const
{
	return (Direction % FCubeCoord::HexDirectionCount + FCubeCoord::HexDirectionCount) % FCubeCoord::HexDirectionCount;
}

FVector2D FDirectionModeTargetTileSelector::GetHexDirectionVector(const int32 Direction) const
{
	const FVector DirectionLocation = FCubeCoord::CubeCoordToWorldCoord(FCubeCoord::GetDirection(NormalizeHexDirection(Direction)));
	return FVector2D(DirectionLocation.X, DirectionLocation.Y).GetSafeNormal();
}

int32 FDirectionModeTargetTileSelector::FindClosestHexDirection(const FVector2D& DesiredDirection) const
{
	int32 ClosestDirection = 0;
	float BestDot = TNumericLimits<float>::Lowest();
	for (int32 Direction = 0; Direction < FCubeCoord::HexDirectionCount; ++Direction)
	{
		const float Dot = FVector2D::DotProduct(DesiredDirection, GetHexDirectionVector(Direction));
		if (Dot > BestDot)
		{
			BestDot = Dot;
			ClosestDirection = Direction;
		}
	}
	return ClosestDirection;
}

int32 FDirectionModeTargetTileSelector::FindClosestHexDirectionBoundary(const FVector2D& DesiredDirection) const
{
	int32 ClosestUpperDirection = 0;
	float BestDot = TNumericLimits<float>::Lowest();
	for (int32 Direction = 0; Direction < FCubeCoord::HexDirectionCount; ++Direction)
	{
		const FVector2D BoundaryDirection = (GetHexDirectionVector(Direction) + GetHexDirectionVector(Direction - 1)).GetSafeNormal();
		const float Dot = FVector2D::DotProduct(DesiredDirection, BoundaryDirection);
		if (Dot > BestDot)
		{
			BestDot = Dot;
			ClosestUpperDirection = Direction;
		}
	}
	return ClosestUpperDirection;
}

void FDirectionModeTargetTileSelector::GetSelectedDirections(const ATile* CurrentTile, const APlayerController* PlayerController, TArray<int32>& OutDirections) const
{
	OutDirections.Reset();

	FHitResult HitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult);
	if (!HitResult.IsValidBlockingHit())
	{
		return;
	}

	// 현재 서있는 위치와 마우스에서 라인트레이스를 통해 가져온 위치에서 Z축을 빼고 방향 벡터를 계산합니다.
	const FVector CurrentLocation = CurrentTile->GetActorLocation();
	const FVector2D DesiredDirection(HitResult.ImpactPoint.X - CurrentLocation.X, HitResult.ImpactPoint.Y - CurrentLocation.Y);
	if (DesiredDirection.IsNearlyZero())
	{
		return;
	}
	
	// 6개의 방향 중 가장 가까운 방향으로 스냅, 그 방향을 중심으로 해 반시계 방향으로 회전하며 선택합니다.
	const int32 ClampedDirectionCount = FMath::Clamp(DirectionCount, 1, FCubeCoord::HexDirectionCount);
	OutDirections.Reserve(ClampedDirectionCount);
	
	if (ClampedDirectionCount % 2 == 1)
	{
		const int32 CenterDirection = FindClosestHexDirection(DesiredDirection);
		const int32 HalfDirectionCount = ClampedDirectionCount / 2;
		for (int32 Offset = -HalfDirectionCount; Offset <= HalfDirectionCount; ++Offset)
		{
			OutDirections.Add(NormalizeHexDirection(CenterDirection + Offset));
		}
		return;
	}

	const int32 UpperDirection = FindClosestHexDirectionBoundary(DesiredDirection);
	const int32 HalfDirectionCount = ClampedDirectionCount / 2;
	for (int32 Offset = -HalfDirectionCount; Offset <= HalfDirectionCount - 1; ++Offset)
	{
		OutDirections.Add(NormalizeHexDirection(UpperDirection + Offset));
	}
}
