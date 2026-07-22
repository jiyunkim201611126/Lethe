// Copyright JETBLU, Inc. All Rights Reserved.

#include "ActorSelectorComponent.h"

#include "Lethe/Lethe.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

UActorSelectorComponent::UActorSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UActorSelectorComponent::HighlightActorsByMouse(const TArray<AActor*>& Actors, const bool bIsTile)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}
	
	LastMouseHoveredActors = CurrentMouseHoveredActors;
	CurrentMouseHoveredActors.Reset();
	for (AActor* Actor : Actors)
	{
		if (Actor && Actor->Implements<UHighlightInterface>())
		{
			CurrentMouseHoveredActors.Add(Actor);
		}
	}

	bool bMayHaveDetectedOtherTile = false;

	for (const auto& LastMouseHoveredActor : LastMouseHoveredActors)
	{
		if (!LastMouseHoveredActor)
		{
			continue;
		}
		
		if (!CurrentMouseHoveredActors.Contains(LastMouseHoveredActor))
		{
			bMayHaveDetectedOtherTile = true;
			
			IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredActor.GetObject());
		}
	}

	for (auto& CurrentMouseHoveredActor : CurrentMouseHoveredActors)
	{
		if (!CurrentMouseHoveredActor)
		{
			continue;
		}
		
		if (!LastMouseHoveredActors.Contains(CurrentMouseHoveredActor))
		{
			bMayHaveDetectedOtherTile = true;
			
			IHighlightInterface::Execute_HighlightActorByMouse(CurrentMouseHoveredActor.GetObject());
		}
	}
	
	if (bIsTile && bMayHaveDetectedOtherTile)
	{
		OnDetectedOtherTile.ExecuteIfBound();
	}
}

void UActorSelectorComponent::HighlightTilesByMouse(const TArray<ATile*>& Tiles)
{
	TArray<AActor*> Actors;
	Actors.Append(Tiles);
	HighlightActorsByMouse(Actors, true);
}

void UActorSelectorComponent::UnhighlightActorByMouse()
{
	for (const auto& LastMouseHoveredActor : LastMouseHoveredActors)
	{
		if (LastMouseHoveredActor)
		{
			IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredActor.GetObject());
		}
	}
	LastMouseHoveredActors.Reset();
	for (const auto& CurrentMouseHoveredActor : CurrentMouseHoveredActors)
	{
		if (CurrentMouseHoveredActor)
		{
			IHighlightInterface::Execute_UnhighlightActorByMouse(CurrentMouseHoveredActor.GetObject());
		}
	}
	CurrentMouseHoveredActors.Reset();
}

void UActorSelectorComponent::HighlightActorsByAbility(const TArray<ATile*>& Tiles, AActor* AbilityOwner)
{
	UnhighlightActorsByAbility();
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		for (ATile* Tile : Tiles)
		{
			if (!Tile)
			{
				continue;
			}
			
			// 타일 위에 카드 주인이 있다면 검은색, 다른 게 있다면 초록색으로, 아무것도 없다면 파란색으로 아웃라인을 표시합니다.
			int32 OutlineColor;
			if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(Tile))
			{
				OutlineColor = ActorOnTile == AbilityOwner ? CUSTOM_DEPTH_BLACK : CUSTOM_DEPTH_GREEN;
			}
			else
			{
				OutlineColor = CUSTOM_DEPTH_BLUE;
			}

			IHighlightInterface::Execute_HighlightActorByAbility(Tile, OutlineColor);
			CurrentHighlightedTilesByAbility.Add(Tile);
		}
	}
	
	CurrentHighlightedCharacterByAbility = AbilityOwner;
	IHighlightInterface::Execute_HighlightActorByAbility(CurrentHighlightedCharacterByAbility.GetObject(), INDEX_NONE);
}

void UActorSelectorComponent::UnhighlightActorsByAbility()
{
	for (const auto& HighlightedTile : CurrentHighlightedTilesByAbility)
	{
		if (HighlightedTile)
		{
			IHighlightInterface::Execute_UnhighlightActorByAbility(HighlightedTile.GetObject());
		}
	}
	CurrentHighlightedTilesByAbility.Reset();
	
	if (CurrentHighlightedCharacterByAbility)
	{
		IHighlightInterface::Execute_UnhighlightActorByAbility(CurrentHighlightedCharacterByAbility.GetObject());
		CurrentHighlightedCharacterByAbility = nullptr;
	}
}

void UActorSelectorComponent::GetTileHitResult(FTileHitResult& TileHitResult) const
{
	TileHitResult.Tile = nullptr;
	TileHitResult.ActorOnTile = nullptr;
	
	if (const APlayerController* PlayerController = GetOwner<APlayerController>())
	{
		FHitResult HitResult;
		PlayerController->GetHitResultUnderCursor(ECC_Tile, false, HitResult);

		if (HitResult.IsValidBlockingHit())
		{
			if (ATile* HitTile = Cast<ATile>(HitResult.GetActor()))
			{
				const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
				ATile* TopTile = HitTile->GetTopTile();
				if (TileManagerSubsystem && TopTile)
				{
					TileHitResult.Tile = TopTile;
					TileHitResult.ActorOnTile = TileManagerSubsystem->GetActorOnTile(TopTile);
					TileHitResult.ImpactPoint = HitResult.ImpactPoint;
				}
			}
		}
	}
}

bool UActorSelectorComponent::TryGetTilesByRangeFromTile(const ATile* Tile, const FBFSRange& InRange, const ETileRangeQueryType QueryType, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!Tile || !TileManagerSubsystem)
	{
		return false;
	}
	
	TSet<FCubeCoord> SelectedCoords;
	TileManagerSubsystem->TileBFS(Tile->GetCubeCoord(), InRange.Distance, InRange.BFSType, SelectedCoords,
		[TileManagerSubsystem, QueryType](const FTileData* CurrentTileData, const FTileData* NextTileData)
		{
			if (QueryType == ETileRangeQueryType::PlayerMove)
			{
				if (!NextTileData || !NextTileData->TopTile.IsValid())
				{
					return false;
				}
				
				if (const AActor* ActorOnNextTile = TileManagerSubsystem->GetActorOnTile(NextTileData->TopTile.Get()))
				{
					// 아군 캐릭터가 아닌 액터가 서있다면 해당 좌표는 이동 가능 경로에서 제외됩니다.
					if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(ActorOnNextTile))
					{
						return CombatInterface->GetTeamSide() == ETeamSide::Player;
					}
					return false;
				}
			}
			return true;
		},
		[TileManagerSubsystem, QueryType](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
		{
			if (QueryType == ETileRangeQueryType::PlayerMove)
			{
				// 목적지 좌표에 아군 캐릭터가 서있다면, 해당 캐릭터와 스왑 가능 여부를 판별합니다.
				if (!TileData || !TileData->TopTile.IsValid())
				{
					return false;
				}

				if (const AActor* ActorOnNextTile = TileManagerSubsystem->GetActorOnTile(TileData->TopTile.Get()))
				{
					if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(ActorOnNextTile))
					{
						if (CombatInterface->GetTeamSide() == ETeamSide::Player && Depth <= CombatInterface->GetMoveRange())
						{
							return true;
						}
					}
					return false;
				}
			}
			return true;
		});

	for (const FCubeCoord& SelectedCoord : SelectedCoords)
	{
		if (ATile* SelectedTile = TileManagerSubsystem->GetTile(SelectedCoord))
		{
			OutTiles.Add(SelectedTile);
		}
	}

	return !OutTiles.IsEmpty();
}

bool UActorSelectorComponent::TryGetTilesByRangeFromActor(const AActor* Actor, const FBFSRange& InRange, const ETileRangeQueryType QueryType, TArray<ATile*>& OutTiles) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return false;
	}

	const ATile* Tile = TileManagerSubsystem->GetTileUnderActor(Actor);
	if (!Tile)
	{
		return false;
	}

	return TryGetTilesByRangeFromTile(Tile, InRange, QueryType, OutTiles);
}
