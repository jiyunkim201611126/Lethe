// Copyright JETBLU, Inc. All Rights Reserved.

#include "TileSelectorComponent.h"

#include "Lethe/Lethe.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

UTileSelectorComponent::UTileSelectorComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UTileSelectorComponent::HighlightTileByMouse(AActor* Tile)
{
	if (Tile)
	{
		LastMouseHoveredTile = CurrentMouseHoveredTile;
		CurrentMouseHoveredTile = Tile;

		if (LastMouseHoveredTile != CurrentMouseHoveredTile)
		{
			if (LastMouseHoveredTile)
			{
				IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredTile.GetObject());
			}
			if (CurrentMouseHoveredTile)
			{
				IHighlightInterface::Execute_HighlightActorByMouse(CurrentMouseHoveredTile.GetObject());
			}
			
			if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
			{
				const AActor* LastActor = TileManagerSubsystem->GetActorOnTile(Cast<ATile>(LastMouseHoveredTile.GetObject()));
				const AActor* CurrentActor = TileManagerSubsystem->GetActorOnTile(Cast<ATile>(CurrentMouseHoveredTile.GetObject()));
				if (LastActor != CurrentActor)
				{
					OnDetectedOtherTile.ExecuteIfBound(LastActor, CurrentActor);
				}
			}
		}
	}
}

void UTileSelectorComponent::UnhighlightTileByMouse()
{
	if (LastMouseHoveredTile)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredTile.GetObject());
		LastMouseHoveredTile = nullptr;
	}
	if (CurrentMouseHoveredTile)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(CurrentMouseHoveredTile.GetObject());
		CurrentMouseHoveredTile = nullptr;
	}
}

void UTileSelectorComponent::HighlightTileByCard(const TArray<ATile*>& Tiles, const AActor* CardOwner)
{
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		for (ATile* Tile : Tiles)
		{
			// 타일 위에 카드 주인이 있다면 검은색, 다른 게 있다면 초록색으로, 아무것도 없다면 파란색으로 아웃라인을 표시합니다.
			int32 OutlineColor;
			if (const AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(Tile))
			{
				OutlineColor = ActorOnTile == CardOwner ? CUSTOM_DEPTH_BLACK : CUSTOM_DEPTH_GREEN;
			}
			else
			{
				OutlineColor = CUSTOM_DEPTH_BLUE;
			}
			
			IHighlightInterface::Execute_HighlightActorByCard(Tile, OutlineColor);
			CurrentHighlightedByCardTiles.Emplace(Tile);
		}
	}
}

void UTileSelectorComponent::UnhighlightTileByCard()
{
	for (TScriptInterface<IHighlightInterface> HighlightTile : CurrentHighlightedByCardTiles)
	{
		IHighlightInterface::Execute_UnhighlightActorByCard(HighlightTile.GetObject());
	}
	CurrentHighlightedByCardTiles.Reset();
}

AActor* UTileSelectorComponent::GetActorOnTileUnderCursor() const
{
	if (const APlayerController* PlayerController = GetOwner<APlayerController>())
	{
		FHitResult Hit;
		PlayerController->GetHitResultUnderCursor(ECC_Tile, false, Hit);

		if (Hit.IsValidBlockingHit())
		{
			if (ATile* HitTile = Cast<ATile>(Hit.GetActor()))
			{
				const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
				const ATile* TopTile = HitTile->GetTopTile();
				if (TileManagerSubsystem && TopTile)
				{
					return TileManagerSubsystem->GetActorOnTile(TopTile);
				}
			}
		}
	}

	return nullptr;
}

bool UTileSelectorComponent::TryGetTilesByDepth(TArray<ATile*>& OutTiles, const AActor* ActorOnTile, const FAbilityRange& InRange) const
{
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* Tile = TileManagerSubsystem->GetTileUnderActor(ActorOnTile))
		{
			TSet<FCubeCoord> SelectedCoords;
			TileManagerSubsystem->TileBFS(Tile->GetCubeCoord(), InRange.Depth, InRange.BFSType, SelectedCoords,
				[]()
				{
					return true;
				},
				[](const FTileData* TileData, int32 Depth)
				{
					return true;
				});

			for (const FCubeCoord& SelectedCoord : SelectedCoords)
			{
				if (ATile* SelectedTile = TileManagerSubsystem->GetTile(SelectedCoord))
				{
					OutTiles.Emplace(SelectedTile);
				}
			}
		}
	}

	return !OutTiles.IsEmpty();
}

void UTileSelectorComponent::Reset()
{
	LastMouseHoveredTile = nullptr;
	CurrentMouseHoveredTile = nullptr;
}

