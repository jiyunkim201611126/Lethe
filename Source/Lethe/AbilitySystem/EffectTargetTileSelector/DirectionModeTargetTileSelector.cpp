// Copyright JETBLU, Inc. All Rights Reserved.

#include "DirectionModeTargetTileSelector.h"

#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

void UDirectionModeTargetTileSelector::GetSelectCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}

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

	const ATile* StandingTile = TileManagerSubsystem->GetTileUnderActor(AvatarActor);
	if (!StandingTile)
	{
		return;
	}

	// 현재 서있는 위치와 마우스에서 라인트레이스를 통해 가져온 위치에서 Z축을 빼고 방향 벡터를 계산합니다.
	const FVector StandingLocation = StandingTile->GetActorLocation();
	const FVector2D DesiredDirection(HitResult.ImpactPoint.X - StandingLocation.X, HitResult.ImpactPoint.Y - StandingLocation.Y);
	if (DesiredDirection.IsNearlyZero())
	{
		return;
	}

	// 원하는 방향 개수만큼 방향을 선택합니다.
	TArray<int32> SelectedDirections;
	GetSelectedDirections(DesiredDirection.GetSafeNormal(), SelectedDirections);

	const FCubeCoord CenterCoord = StandingTile->GetCubeCoord();
	const int32 MaxRange = RangeType == ERangeType::Melee ? 1 : FMath::Max(1, RangeEnforceValue);
	for (const int32 Direction : SelectedDirections)
	{
		for (int32 Distance = 1; Distance <= MaxRange; ++Distance)
		{
			const FCubeCoord DirectionOffset = FCubeCoord::GetDirection(Direction);
			const FCubeCoord TargetCoord(
				CenterCoord.Q + DirectionOffset.Q * Distance,
				CenterCoord.R + DirectionOffset.R * Distance,
				CenterCoord.S + DirectionOffset.S * Distance);
			
			if (ATile* Tile = TileManagerSubsystem->GetTile(TargetCoord))
			{
				OutTiles.AddUnique(Tile);
			}

			if (RangeType == ERangeType::Melee)
			{
				break;
			}
		}
	}
}

void UDirectionModeTargetTileSelector::GetTargetCandidateTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();
	
	if (!AvatarActor || !PlayerController)
	{
		return;
	}

	FHitResult TileHitResult;
	PlayerController->GetHitResultUnderCursor(ECC_Tile, false, TileHitResult);
	if (!TileHitResult.IsValidBlockingHit())
	{
		return;
	}

	ATile* HitTile = Cast<ATile>(TileHitResult.GetActor());
	if (!HitTile)
	{
		return;
	}

	switch (RangeType)
	{
	case ERangeType::Melee:
	case ERangeType::ParabolaRanged:
		OutTiles.Add(HitTile);
		break;
	case ERangeType::StraightRanged:
		{
			const FVector StartLocation = AvatarActor->GetActorLocation();
			FVector EndLocation = HitTile->GetActorLocation();
			EndLocation.Z = StartLocation.Z;
			
			FHitResult PawnHitResult;
			FCollisionQueryParams CollisionQueryParams;
			CollisionQueryParams.AddIgnoredActor(AvatarActor);
			if (AvatarActor->GetWorld()->LineTraceSingleByChannel(PawnHitResult, StartLocation, EndLocation, ECC_Pawn, CollisionQueryParams))
			{
				const AActor* HitActor = PawnHitResult.GetActor();
				if (HitActor->Implements<UCombatInterface>())
				{
					if (const UTileManagerSubsystem* TileManagerSubsystem = AvatarActor->GetWorld()->GetSubsystem<UTileManagerSubsystem>())
					{
						OutTiles.Add(TileManagerSubsystem->GetTileUnderActor(HitActor));
					}
				}
				else
				{
					OutTiles.Add(HitTile);
				}
			}
		}
		break;
	}
}

void UDirectionModeTargetTileSelector::GetTargetTiles(const AActor* AvatarActor, const APlayerController* PlayerController, TArray<ATile*>& OutTiles) const
{
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
