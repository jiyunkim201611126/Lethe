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

void UActorSelectorComponent::HighlightActorByMouse(AActor* Actor, const bool bTransparent)
{
	LastMouseHoveredActor = CurrentMouseHoveredActor;
	CurrentMouseHoveredActor = Actor;

	if (LastMouseHoveredActor != CurrentMouseHoveredActor)
	{
		if (LastMouseHoveredActor)
		{
			IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredActor.GetObject());
		}
		if (CurrentMouseHoveredActor)
		{
			if (bTransparent)
			{
				IHighlightInterface::Execute_HighlightActorTransparentByMouse(CurrentMouseHoveredActor.GetObject());
			}
			else
			{
				IHighlightInterface::Execute_HighlightActorByMouse(CurrentMouseHoveredActor.GetObject());
			}
		}
		
		if (Actor && Actor->IsA<ATile>())
		{
			// 새롭게 검출된 액터가 타일인 경우 들어오는 분기입니다.
			if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
			{
				AActor* LastActor = TileManagerSubsystem->GetActorOnTile(Cast<ATile>(LastMouseHoveredActor.GetObject()));
				AActor* CurrentActor = TileManagerSubsystem->GetActorOnTile(Cast<ATile>(CurrentMouseHoveredActor.GetObject()));
				OnDetectedOtherTile.ExecuteIfBound(LastActor, CurrentActor);
			}
		}
	}
}

void UActorSelectorComponent::UnhighlightActorByMouse()
{
	if (LastMouseHoveredActor)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(LastMouseHoveredActor.GetObject());
		LastMouseHoveredActor = nullptr;
	}
	if (CurrentMouseHoveredActor)
	{
		IHighlightInterface::Execute_UnhighlightActorByMouse(CurrentMouseHoveredActor.GetObject());
		CurrentMouseHoveredActor = nullptr;
	}
}

void UActorSelectorComponent::HighlightActorsByAbility(const TArray<ATile*>& Tiles, AActor* AbilityOwner)
{
	UnhighlightActorsByAbility();
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		for (ATile* Tile : Tiles)
		{
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
	
	IHighlightInterface::Execute_HighlightActorByAbility(AbilityOwner, INDEX_NONE);
	CurrentHighlightedCharacterByAbility = AbilityOwner;
}

void UActorSelectorComponent::UnhighlightActorsByAbility()
{
	for (TScriptInterface<IHighlightInterface> HighlightedTile : CurrentHighlightedTilesByAbility)
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

void UActorSelectorComponent::GetTileAndActorUnderCursor(FTileAndActor& TileAndActor) const
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
				ATile* TopTile = HitTile->GetTopTile();
				if (TileManagerSubsystem && TopTile)
				{
					TileAndActor.Tile = TopTile;
					TileAndActor.Actor = TileManagerSubsystem->GetActorOnTile(TopTile);
				}
			}
		}
	}
}

bool UActorSelectorComponent::TryGetTilesByRange(TArray<ATile*>& OutTiles, const AActor* ActorOnTile, const FBFSRange& InRange, const ETileRangeQueryType QueryType) const
{
	OutTiles.Reset();
	if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (const ATile* Tile = TileManagerSubsystem->GetTileUnderActor(ActorOnTile))
		{
			TSet<FCubeCoord> SelectedCoords;
			TileManagerSubsystem->TileBFS(Tile->GetCubeCoord(), InRange.Distance, InRange.BFSType, SelectedCoords,
				[TileManagerSubsystem, QueryType](const FTileData* CurrentTileData, const FTileData* NextTileData)
				{
					if (QueryType == ETileRangeQueryType::PlayerMove)
					{
						// 아군 캐릭터가 아닌 액터가 서있다면 해당 좌표는 이동 가능 경로에서 제외됩니다.
						if (!NextTileData || !NextTileData->TopTile.IsValid())
						{
							return false;
						}
						
						if (const AActor* ActorOnNextTile = TileManagerSubsystem->GetActorOnTile(NextTileData->TopTile.Get()))
						{
							if (const ICombatInterface* CombatInterface = Cast<ICombatInterface>(ActorOnNextTile))
							{
								return CombatInterface->GetTeamSide() == ETeamSide::Player;
							}
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
		}
	}

	return !OutTiles.IsEmpty();
}
