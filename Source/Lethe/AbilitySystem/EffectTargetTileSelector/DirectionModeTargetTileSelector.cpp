// Copyright JETBLU, Inc. All Rights Reserved.

#include "DirectionModeTargetTileSelector.h"

#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

FDirectionModeTargetTileSelector::FDirectionModeTargetTileSelector()
{
	TargetTeamRelation = ETargetTeamRelation::AllSides;
}

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
	FilterTargetTilesByTeamRelation(Context);
}

void FDirectionModeTargetTileSelector::GetSelectCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	const FCubeCoord CenterCoord = Context.CurrentTile->GetCubeCoord();

	if (RangeType == ERangeType::Melee)
	{
		// 근접 타입인 경우 모든 방향 한 칸 앞을 모두 OutTiles에 추가합니다.
		for (int32 Direction = 1; Direction <= FCubeCoord::HexDirectionCount; ++Direction)
		{
			const FCubeCoord DirectionOffset = FCubeCoord::GetDirection(Direction);
			const FCubeCoord SelectCandidateCoord(
				CenterCoord.Q + DirectionOffset.Q,
				CenterCoord.R + DirectionOffset.R,
				CenterCoord.S + DirectionOffset.S);

			Context.OutSelectCandidateTiles.Add(Context.TileManagerSubsystem->GetTile(SelectCandidateCoord));
		}
	}
	else
	{
		// 필요한 방향들을 가져옵니다.
		TArray<int32> SelectedDirections;
		GetSelectedDirections(Context.CurrentTile, Context.TargetingIntent, SelectedDirections);

		// 지정된 방향을 모두 순회하며, 해당 방향으로 1칸씩 뻗어나가면서 모든 타일을 OutTiles에 추가합니다.
		const int32 MaxRangeDistance = FMath::Max(1, RangeDistance);
		for (const int32 Direction : SelectedDirections)
		{
			for (int32 Distance = 1; Distance <= MaxRangeDistance; ++Distance)
			{
				const FCubeCoord DirectionOffset = FCubeCoord::GetDirection(Direction);
				const FCubeCoord SelectCandidateCoord(
					CenterCoord.Q + DirectionOffset.Q * Distance,
					CenterCoord.R + DirectionOffset.R * Distance,
					CenterCoord.S + DirectionOffset.S * Distance);

				// 인덱스가 곧 CurrentTile과의 거리를 나타내므로, nullptr이더라도 추가합니다.
				Context.OutSelectCandidateTiles.Add(Context.TileManagerSubsystem->GetTile(SelectCandidateCoord));
			}
		}
	}
}

void FDirectionModeTargetTileSelector::GetTargetCandidateTiles(FEffectTargetTileSelectorContext& Context) const
{
	TArray<FResolvedPrimaryTargetTile> OutData;
	switch (RangeType)
	{
	case ERangeType::Melee:
		HandleMeleeRanged(Context, OutData);
		break;
	case ERangeType::ParabolaRanged:
		HandleParabolaRanged(Context, OutData);
		break;
	case ERangeType::StraightRanged:
		HandleStraightRanged(Context, OutData);
		break;
	}
	HandleAdditionalRanges(Context, OutData);
}

void FDirectionModeTargetTileSelector::HandleMeleeRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const
{
	if (Context.OutSelectCandidateTiles.Contains(Context.TargetingIntent.HitTile))
	{
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		FTargetSelectResult& PrimaryTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
		PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Primary;

		ATile* TargetTile = Context.TargetingIntent.HitTile;
		FSelectedTarget& TargetSelectResult = PrimaryTargets.Targets.AddDefaulted_GetRef();
		TargetSelectResult.TargetTile = TargetTile;
		TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(TargetTile);

		const FCubeCoord DirectionCoord = Context.TargetingIntent.HitTile->GetCubeCoord() - Context.CurrentTile->GetCubeCoord();
		const int32 Direction = FCubeCoord::GetDirection(DirectionCoord);

		FResolvedPrimaryTargetTile& AddedData = OutTargetTiles.AddDefaulted_GetRef();
		AddedData.Tile = Context.TargetingIntent.HitTile;
		AddedData.Direction = Direction;
		AddedData.Distance = 1;
	}
}

void FDirectionModeTargetTileSelector::HandleParabolaRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const
{
	// AvatarActor가 서있는 타일과 마우스 위치까지의 거리를 계산합니다.
	const FVector CurrentTileLocation = Context.CurrentTile->GetActorLocation();
	const FVector HitLocation = Context.TargetingIntent.ImpactPoint;
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
	FTargetSelectResult& PrimaryTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
	PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Primary;

	// AdditionalRange 계산을 위해 Direction이 필요하므로, 여기서 다시 가져옵니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(Context.CurrentTile, Context.TargetingIntent, SelectedDirections);

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
		FSelectedTarget& TargetSelectResult = PrimaryTargets.Targets.AddDefaulted_GetRef();
		TargetSelectResult.TargetTile = TargetTile;
		TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(TargetTile);

		FResolvedPrimaryTargetTile& AddedData = OutTargetTiles.AddDefaulted_GetRef();
		AddedData.Tile = TargetTile;
		AddedData.Direction = Direction;
		AddedData.Distance = HitTileDistance;

		TileIndex += MaxRangeDistance;
	}
}

void FDirectionModeTargetTileSelector::HandleStraightRanged(FEffectTargetTileSelectorContext& Context, TArray<FResolvedPrimaryTargetTile>& OutTargetTiles) const
{
	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(Context.CurrentTile, Context.TargetingIntent, SelectedDirections);

	const FCubeCoord CenterCoord = Context.CurrentTile->GetCubeCoord();
	const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	FTargetSelectResult& PrimaryTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
	PrimaryTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Primary;
	PrimaryTargets.Targets.Reserve(SelectedDirections.Num() * MaxRangeDistance);

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

			if (ATile* TargetTile = Context.TileManagerSubsystem->GetTile(TargetCoord))
			{
				if (const AActor* ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(TargetTile))
				{
					if (ActorOnTile->Implements<UCombatInterface>())
					{
						// 전투 가능한 액터가 올라서있다면 타일을 추가합니다.
						FSelectedTarget& TargetSelectResult = PrimaryTargets.Targets.AddDefaulted_GetRef();
						TargetSelectResult.TargetTile = TargetTile;
						TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(TargetTile);

						FResolvedPrimaryTargetTile& AddedData = OutTargetTiles.AddDefaulted_GetRef();
						AddedData.Tile = TargetTile;
						AddedData.Direction = Direction;
						AddedData.Distance = Distance;
					}
					else
					{
						// 전투할 수 없는 액터가 올라서있다면 타일은 유지하고 Actor만 비웁니다.
						FSelectedTarget& TargetSelectResult = PrimaryTargets.Targets.AddDefaulted_GetRef();
						TargetSelectResult.TargetTile = TargetTile;
					}
					// 일단 액터를 만났다면 다른 방향을 탐색합니다.
					break;
				}
				// 액터를 찾지 못 했다면 이 방향을 계속해서 나아갑니다.
				continue;
			}

			// 액터를 마주치기 전에 맵 바깥으로 나가버렸다면, nullptr을 추가하고 다른 방향을 탐색합니다.
			PrimaryTargets.Targets.AddDefaulted();
			break;
		}
	}
}

void FDirectionModeTargetTileSelector::HandleAdditionalRanges(FEffectTargetTileSelectorContext& Context, const TArray<FResolvedPrimaryTargetTile>& ResolvedPrimaryTargetTiles) const
{
	if (ResolvedPrimaryTargetTiles.IsEmpty())
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
				FTargetSelectResult& PenetrationTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
				PenetrationTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Penetration;
				for (const FResolvedPrimaryTargetTile& PrimaryTargetTile : ResolvedPrimaryTargetTiles)
				{
					if (!PrimaryTargetTile.Tile)
					{
						continue;
					}

					const FCubeCoord DirectionCoord = FCubeCoord::GetDirection(PrimaryTargetTile.Direction);

					// Primary 타겟 타일과 같은 방향으로 나아가며 타일을 가져옵니다.
					FCubeCoord PenetrationCoord = PrimaryTargetTile.Tile->GetCubeCoord();
					for (int32 EnforceCount = 0; EnforceCount < AdditionalRange.Value; ++EnforceCount)
					{
						PenetrationCoord = PenetrationCoord + DirectionCoord;
						if (ATile* PenetrationTile = Context.TileManagerSubsystem->GetTile(PenetrationCoord))
						{
							FSelectedTarget& TargetSelectResult = PenetrationTargets.Targets.AddDefaulted_GetRef();
							TargetSelectResult.TargetTile = PenetrationTile;
							TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(PenetrationTile);
						}
					}
				}
			}
			break;
		case EAdditionalRangeType::HalfMoon:
			{
				FTargetSelectResult& HalfMoonTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
				HalfMoonTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_HalfMoon;
				for (const FResolvedPrimaryTargetTile& PrimaryTargetTile : ResolvedPrimaryTargetTiles)
				{
					if (!PrimaryTargetTile.Tile)
					{
						continue;
					}

					// 타겟 타일의 정보를 토대로 시계, 반시계로 확장할 방향을 계산합니다.
					FCubeCoord ClockwiseTargetTileCoord = PrimaryTargetTile.Tile->GetCubeCoord();
					FCubeCoord CounterClockwiseTargetTileCoord = PrimaryTargetTile.Tile->GetCubeCoord();
					const FCubeCoord ClockwiseDirectionCoord = FCubeCoord::GetDirection(PrimaryTargetTile.Direction + 4);
					const FCubeCoord CounterclockwiseDirectionCoord = FCubeCoord::GetDirection(PrimaryTargetTile.Direction + 2);
					for (int32 EnforceCount = 0; EnforceCount < AdditionalRange.Value; ++EnforceCount)
					{
						ClockwiseTargetTileCoord = ClockwiseTargetTileCoord + ClockwiseDirectionCoord;
						CounterClockwiseTargetTileCoord = CounterClockwiseTargetTileCoord + CounterclockwiseDirectionCoord;
						if (ATile* ClockwiseTargetTile = Context.TileManagerSubsystem->GetTile(ClockwiseTargetTileCoord))
						{
							FSelectedTarget& TargetSelectResult = HalfMoonTargets.Targets.AddDefaulted_GetRef();
							TargetSelectResult.TargetTile = ClockwiseTargetTile;
							TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(ClockwiseTargetTile);
						}
						if (ATile* CounterclockwiseTargetTile = Context.TileManagerSubsystem->GetTile(CounterClockwiseTargetTileCoord))
						{
							FSelectedTarget& TargetSelectResult = HalfMoonTargets.Targets.AddDefaulted_GetRef();
							TargetSelectResult.TargetTile = CounterclockwiseTargetTile;
							TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(CounterclockwiseTargetTile);
						}
					}
				}
			}
			break;
		case EAdditionalRangeType::Spread:
			{
				FTargetSelectResult& SpreadTargets = Context.OutTargetTileResults.AddDefaulted_GetRef();
				SpreadTargets.TargetGroupTag = LetheGameplayTags.TargetTileGroup_Spread;
				for (const FResolvedPrimaryTargetTile& PrimaryTargetTile : ResolvedPrimaryTargetTiles)
				{
					if (!PrimaryTargetTile.Tile)
					{
						continue;
					}

					// 타겟 타일의 정보를 토대로 퍼져나가며 확장할 방향을 계산합니다.
					FCubeCoord SpreadLeftTargetTileCoord = PrimaryTargetTile.Tile->GetCubeCoord();
					FCubeCoord SpreadRightTargetTileCoord = PrimaryTargetTile.Tile->GetCubeCoord();
					const FCubeCoord LeftDirectionCoord = FCubeCoord::GetDirection(PrimaryTargetTile.Direction + 1);
					const FCubeCoord RightDirectionCoord = FCubeCoord::GetDirection(PrimaryTargetTile.Direction - 1);
					for (int32 EnforceCount = 0; EnforceCount < AdditionalRange.Value; ++EnforceCount)
					{
						SpreadLeftTargetTileCoord = SpreadLeftTargetTileCoord + LeftDirectionCoord;
						SpreadRightTargetTileCoord = SpreadRightTargetTileCoord + RightDirectionCoord;
						if (ATile* SpreadLeftTargetTile = Context.TileManagerSubsystem->GetTile(SpreadLeftTargetTileCoord))
						{
							FSelectedTarget& TargetSelectResult = SpreadTargets.Targets.AddDefaulted_GetRef();
							TargetSelectResult.TargetTile = SpreadLeftTargetTile;
							TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(SpreadLeftTargetTile);
						}
						if (ATile* SpreadRightTargetTile = Context.TileManagerSubsystem->GetTile(SpreadRightTargetTileCoord))
						{
							FSelectedTarget& TargetSelectResult = SpreadTargets.Targets.AddDefaulted_GetRef();
							TargetSelectResult.TargetTile = SpreadRightTargetTile;
							TargetSelectResult.ActorOnTile = Context.TileManagerSubsystem->GetActorOnTile(SpreadRightTargetTile);
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

FVector2D FDirectionModeTargetTileSelector::GetHexDirectionVector(const int32 Direction) const
{
	const FVector DirectionLocation = FCubeCoord::CubeCoordToWorldCoord(FCubeCoord::GetDirection(Direction));
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

void FDirectionModeTargetTileSelector::GetSelectedDirections(const ATile* CurrentTile, const FTargetingIntent& TargetingIntent, TArray<int32>& OutDirections) const
{
	OutDirections.Reset();

	// 현재 서있는 위치와 마우스에서 라인트레이스를 통해 가져온 위치에서 Z축을 빼고 방향 벡터를 계산합니다.
	const FVector CurrentLocation = CurrentTile->GetActorLocation();
	const FVector2D DesiredDirection(TargetingIntent.ImpactPoint.X - CurrentLocation.X, TargetingIntent.ImpactPoint.Y - CurrentLocation.Y);
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
			OutDirections.Add(FCubeCoord::NormalizeHexDirection(CenterDirection + Offset));
		}
		return;
	}

	const int32 UpperDirection = FindClosestHexDirectionBoundary(DesiredDirection);
	const int32 HalfDirectionCount = ClampedDirectionCount / 2;
	for (int32 Offset = -HalfDirectionCount; Offset <= HalfDirectionCount - 1; ++Offset)
	{
		OutDirections.Add(FCubeCoord::NormalizeHexDirection(UpperDirection + Offset));
	}
}
