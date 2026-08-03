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

void UActorSelectorComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	for (auto It = CurrentHighlightedActorsByReason.CreateConstIterator(); It; ++It)
	{
		ClearHighlightedActors(It.Key());
	}
	
	Super::EndPlay(EndPlayReason);
}

bool UActorSelectorComponent::SetHighlightedActors(const EHighlightReason Reason, const TArray<AActor*>& InActors)
{
	TArray<TScriptInterface<IHighlightInterface>> LastHighlightedActors = CurrentHighlightedActorsByReason.FindRef(Reason);
	TArray<TScriptInterface<IHighlightInterface>> CurrentHighlightedActors;
	CurrentHighlightedActors.Reserve(InActors.Num());
	
	for (AActor* Actor : InActors)
	{
		if (Actor && Actor->Implements<UHighlightInterface>())
		{
			CurrentHighlightedActors.AddUnique(Actor);
		}
	}

	bool bChanged = false;

	for (const auto& LastHighlightedActor : LastHighlightedActors)
	{
		if (LastHighlightedActor && !CurrentHighlightedActors.Contains(LastHighlightedActor))
		{
			bChanged = true;
			IHighlightInterface::Execute_Unhighlight(LastHighlightedActor.GetObject(), Reason);
		}
	}

	for (const auto& CurrentHighlightedActor : CurrentHighlightedActors)
	{
		if (CurrentHighlightedActor && !LastHighlightedActors.Contains(CurrentHighlightedActor))
		{
			bChanged = true;
			IHighlightInterface::Execute_Highlight(CurrentHighlightedActor.GetObject(), Reason);
		}
	}

	CurrentHighlightedActorsByReason.Add(Reason, MoveTemp(CurrentHighlightedActors));

	return bChanged;
}

bool UActorSelectorComponent::SetHighlightedTiles(const EHighlightReason Reason, const TArray<ATile*>& InTiles)
{
	TArray<AActor*> Actors;
	Actors.Reserve(InTiles.Num());
	for (ATile* Tile : InTiles)
	{
		Actors.Add(Tile);
	}

	return SetHighlightedActors(Reason, Actors);
}

void UActorSelectorComponent::ClearHighlightedActors(const EHighlightReason Reason)
{
	const TArray<TScriptInterface<IHighlightInterface>> CurrentHighlightedActors = CurrentHighlightedActorsByReason.FindRef(Reason);
	for (const auto& CurrentHighlightedActor : CurrentHighlightedActors)
	{
		if (CurrentHighlightedActor)
		{
			IHighlightInterface::Execute_Unhighlight(CurrentHighlightedActor.GetObject(), Reason);
		}
	}
	CurrentHighlightedActorsByReason.Remove(Reason);
}

void UActorSelectorComponent::ClearAllHighlightedActors()
{
	TArray<EHighlightReason> HighlightReasons;
	CurrentHighlightedActorsByReason.GetKeys(HighlightReasons);
	for (const EHighlightReason HighlightReason : HighlightReasons)
	{
		ClearHighlightedActors(HighlightReason);
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
