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
	
	// 타겟 후보 타일을 가져옵니다.
	TArray<ATile*> OutTargetCandidateTiles;
	GetTargetCandidateTiles(AvatarActor, PlayerController, OutTargetCandidateTiles);

	if (const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		for (ATile* Tile : OutTargetCandidateTiles)
		{
			const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(Tile);
			if (ActorOnTile && ActorOnTile->Implements<UCombatInterface>())
			{
				// 타겟 후보 타일 위에 전투 가능 액터가 있다면 OutTiles에 추가합니다.
				OutTiles.Add(Tile);
			}
		}
	}
}

void UDirectionModeTargetTileSelector::GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	FHitResult HitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult);
	if (!HitResult.IsValidBlockingHit())
	{
		return;
	}

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

	// 현재 서있는 위치와 마우스에서 라인트레이스를 통해 가져온 위치에서 Z축을 빼고 방향 벡터를 계산합니다.
	const FVector CurrentLocation = CurrentTile->GetActorLocation();
	const FVector2D DesiredDirection(HitResult.ImpactPoint.X - CurrentLocation.X, HitResult.ImpactPoint.Y - CurrentLocation.Y);
	if (DesiredDirection.IsNearlyZero())
	{
		return;
	}

	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(DesiredDirection.GetSafeNormal(), SelectedDirections);

	const FCubeCoord CenterCoord = CurrentTile->GetCubeCoord();
	const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);
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
				OutTiles.Add(Tile);
			}
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
		// AvatarActor가 서있는 타일과 마우스 위치까지의 거리 제곱을 계산합니다.
		const FVector CurrentTileLocation = CurrentTile->GetActorLocation();
		const FVector HitLocation = HitResult.ImpactPoint;
		const float DistanceSquared = FVector::DistSquaredXY(CurrentTileLocation, HitLocation);

		// 타일과 타일 사이의 거리 제곱을 계산합니다.
		const float TileWidthInterval = FCubeCoord::GetTileWidthInterval();
		const float TileWidthIntervalSquared = TileWidthInterval * TileWidthInterval;

		// 서있는 타일과 마우스 위치까지의 거리 제곱에서 타일과 타일 사이의 거리 제곱을 빼고, 해당 수치가 음수인 경우 얼리리턴합니다.
		const float AdjustedDistanceSquared = DistanceSquared - TileWidthIntervalSquared;
		if (AdjustedDistanceSquared <= 0.f)
		{
			return;
		}

		// 최대 사거리를 가져오고, 위에서 계산한 수치를 타일과 타잂 사이의 거리 제곱으로 나눈 후 1을 빼서 방향별 가져올 타일 인덱스를 계산합니다.
		const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);
		int32 TileIndex = static_cast<int32>(AdjustedDistanceSquared / TileWidthIntervalSquared) - 1;
		if (MaxRangeDistance <= TileIndex)
		{
			// 최대 사거리를 벗어나 마우스를 둔 경우 얼리리턴합니다.
			return;
		}

		// 선택 후보 타일을 가져옵니다.
		TArray<ATile*> OutSelectCandidateTiles;
		GetSelectCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles);

		// 거리에 알맞는 타일들만 추가합니다.
		while (OutSelectCandidateTiles.IsValidIndex(TileIndex))
		{
			OutTiles.Add(OutSelectCandidateTiles.Last());
			TileIndex += MaxRangeDistance;
		}
	}
}

void UDirectionModeTargetTileSelector::HandleStraightRanged(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	// 선택 후보 타일을 가져옵니다.
	TArray<ATile*> OutSelectCandidateTiles;
	GetSelectCandidateTiles(AvatarActor, PlayerController, OutSelectCandidateTiles);

	const int32 MaxRangeDistance = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeDistance);
	int32 TileIndex = MaxRangeDistance - 1;

	// 선택 후보 타일 중, 방향의 마지막 위치 타일만 가져옵니다.
	TArray<ATile*> DirectionLastTiles;
	while (OutSelectCandidateTiles.IsValidIndex(TileIndex))
	{
		DirectionLastTiles.Add(OutSelectCandidateTiles[TileIndex]);
		TileIndex += MaxRangeDistance;
	}

	// AvatarActor 위치에서 마지막 위치 타일들로 LineTrace를 수행합니다.
	if (const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		const FVector StartLocation = AvatarActor->GetActorLocation();
		FCollisionQueryParams CollisionQueryParams;
		CollisionQueryParams.AddIgnoredActor(AvatarActor);
		for (ATile* DirectionLastTile : DirectionLastTiles)
		{
			FVector EndLocation = DirectionLastTile->GetActorLocation();
			EndLocation.Z = StartLocation.Z;
		
			FHitResult HitResult;
			if (AvatarActor->GetWorld()->LineTraceSingleByChannel(HitResult, StartLocation, EndLocation, ECC_Pawn, CollisionQueryParams))
			{
				const AActor* HitActor = HitResult.GetActor();
				if (HitActor->Implements<UCombatInterface>())
				{
					// 검출된 Pawn이 있다면 해당 Pawn이 서있는 타일을 OutTiles에 추가합니다.
					OutTiles.Add(TileManagerSubsystem->GetTileUnderActor(HitActor));
					continue;
				}
			}
			
			// 검출된 Pawn이 없다면 하이라이팅을 위해 마지막 위치 타일을 OutTiles에 추가합니다.
			OutTiles.Add(DirectionLastTile);
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

void UDirectionModeTargetTileSelector::GetSelectedDirections(const FVector2D& DesiredDirection, TArray<int32>& OutDirections) const
{
	OutDirections.Reset();

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
