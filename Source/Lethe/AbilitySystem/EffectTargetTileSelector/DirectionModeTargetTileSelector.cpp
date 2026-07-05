// Copyright JETBLU, Inc. All Rights Reserved.

#include "DirectionModeTargetTileSelector.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void UDirectionModeTargetTileSelector::GetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutSelectCandidateTiles, TArray<ATile*>& OutTargetCandidateTiles)
{
	OutSelectCandidateTiles.Reset();
	OutTargetCandidateTiles.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}

	GetSelectCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles);
	GetTargetCandidateTiles(AvatarActor, PlayerController, OutTargetCandidateTiles);
}

void UDirectionModeTargetTileSelector::GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}
	
	GetTargetCandidateTiles(AvatarActor, PlayerController, OutTiles);
}

void UDirectionModeTargetTileSelector::GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
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

	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(CurrentTile, PlayerController, SelectedDirections);

	const FCubeCoord CenterCoord = CurrentTile->GetCubeCoord();
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
			OutTiles.Add(TileManagerSubsystem->GetTile(TargetCoord));
		}
	}
}

void UDirectionModeTargetTileSelector::GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	switch (RangeType)
	{
	case ERangeType::Melee:
	case ERangeType::ParabolaRanged:
		HandleMeleeAndParabolaRanged(AvatarActor, PlayerController, OutTiles);
		break;
	case ERangeType::StraightRanged:
		HandleStraightRanged(AvatarActor, PlayerController, OutTiles);
		break;
	}
}

void UDirectionModeTargetTileSelector::HandleMeleeAndParabolaRanged(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
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
	
	FHitResult HitResult;
	if (PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult))
	{
		// AvatarActor가 서있는 타일과 마우스 위치까지의 거리를 계산합니다.
		const FVector CurrentTileLocation = CurrentTile->GetActorLocation();
		const FVector HitLocation = HitResult.ImpactPoint;
		const float Distance = FVector::Dist(CurrentTileLocation, HitLocation);

		// 타일과 타일 사이의 거리를 가져옵니다.
		const float TileWidthInterval = FCubeCoord::GetTileWidthInterval();

		// 거리를 타일 기준으로 계산합니다.
		const int32 TileDistance = FMath::RoundToInt(Distance / TileWidthInterval);

		const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);
		if (!(0 <= TileDistance && TileDistance <= MaxRangeDistance))
		{
			// 사거리를 벗어나 마우스를 둔 경우 얼리리턴합니다.
			return;
		}
		
		// 선택 후보 타일을 가져옵니다.
		TArray<ATile*> OutSelectCandidateTiles;
		GetSelectCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles);

		// 거리에 알맞는 타일들만 추가합니다.
		int32 TileIndex = TileDistance - 1;
		while (OutSelectCandidateTiles.IsValidIndex(TileIndex))
		{
			OutTiles.Add(OutSelectCandidateTiles[TileIndex]);
			TileIndex += MaxRangeDistance;
		}
	}
}

void UDirectionModeTargetTileSelector::HandleStraightRanged(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
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
	
	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(CurrentTile, PlayerController, SelectedDirections);

	const FCubeCoord CenterCoord = CurrentTile->GetCubeCoord();
	const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);

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

			if (ATile* Tile = TileManagerSubsystem->GetTile(TargetCoord))
			{
				if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(Tile))
				{
					if (ActorOnTile->Implements<UCombatInterface>())
					{
						// 전투 가능한 액터가 올라서있다면 타일을 추가합니다.
						OutTiles.Add(Tile);
					}
					else
					{
						// 전투할 수 없는 액터가 올라서있다면 nullptr을 추가합니다.
						OutTiles.Add(nullptr);
					}
					// 일단 액터를 만났다면 다른 방향을 탐색합니다.
					break;
				}
				// 액터를 찾지 못 했다면 이 방향을 계속해서 나아갑니다.
				continue;
			}
			
			// 액터를 마주치기 전에 맵 바깥으로 나가버렸다면, nullptr을 추가하고 다른 방향을 탐색합니다.
			OutTiles.Add(nullptr);
			break;
		}
	}
}

int32 UDirectionModeTargetTileSelector::NormalizeHexDirection(const int32 Direction) const
{
	return (Direction % FCubeCoord::HexDirectionCount + FCubeCoord::HexDirectionCount) % FCubeCoord::HexDirectionCount;
}

FVector2D UDirectionModeTargetTileSelector::GetHexDirectionVector(const int32 Direction) const
{
	const FVector DirectionLocation = FCubeCoord::CubeCoordToWorldCoord(FCubeCoord::GetDirection(NormalizeHexDirection(Direction)));
	return FVector2D(DirectionLocation.X, DirectionLocation.Y).GetSafeNormal();
}

int32 UDirectionModeTargetTileSelector::FindClosestHexDirection(const FVector2D& DesiredDirection) const
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

int32 UDirectionModeTargetTileSelector::FindClosestHexDirectionBoundary(const FVector2D& DesiredDirection) const
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

void UDirectionModeTargetTileSelector::GetSelectedDirections(const ATile* CurrentTile, const APlayerController* PlayerController, TArray<int32>& OutDirections) const
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
	
	const int32 ClampedDirectionCount = FMath::Clamp(DirectionCount, 1, FCubeCoord::HexDirectionCount);
	if (ClampedDirectionCount % 2 == 1)
	{
		// 홀수인 경우, 6개의 방향 중 가장 가까운 방향으로 스냅, 그 주변 방향을 함께 선택합니다.
		const int32 CenterDirection = FindClosestHexDirection(DesiredDirection);
		const int32 HalfDirectionCount = ClampedDirectionCount / 2;
		for (int32 Offset = HalfDirectionCount; Offset >= -HalfDirectionCount; --Offset)
		{
			OutDirections.Add(NormalizeHexDirection(CenterDirection + Offset));
		}
		return;
	}

	// 짝수인 경우, 6개의 경계 방향 중 가장 가까운 경계 방향으로 스냅, 그리고 그 바로 반시계 방향 옆 방향을 기준으로 Direction을 가져옵니다.
	// 해당 Direction을 기준으로 반시계 방향 (DirectionCount / 2 - 1)칸부터 시계 방향으로 회전하여 DirectionCount만큼 선택합니다.
	const int32 UpperDirection = FindClosestHexDirectionBoundary(DesiredDirection);
	const int32 HalfDirectionCount = ClampedDirectionCount / 2;
	for (int32 Offset = HalfDirectionCount - 1; Offset >= -HalfDirectionCount; --Offset)
	{
		OutDirections.Add(NormalizeHexDirection(UpperDirection + Offset));
	}
}
