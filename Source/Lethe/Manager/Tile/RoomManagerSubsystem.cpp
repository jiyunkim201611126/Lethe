// Copyright JETBLU, Inc. All Rights Reserved.

#include "RoomManagerSubsystem.h"

#include "TileManagerSubsystem.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"

void URoomManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	RoomDataMap.Empty();
}

void URoomManagerSubsystem::SetRoomData(TMap<int32, FRoomData>&& InRoomData)
{
	RoomDataMap = MoveTemp(InRoomData);
}

void URoomManagerSubsystem::NotifyActorTileChanged(const AActor* InActor, const ATile* OldTile, const ATile* NewTile)
{
	if (!NewTile)
	{
		// OldTile은 nullptr일 수 있습니다.
		return;
	}

	if (InActor->Implements<UPlayerCharacterInterface>())
	{
		UpdatePlayerRoomState(OldTile, NewTile);
	}
}

void URoomManagerSubsystem::RevealEnemyTile(ATile* InTile) const
{
	InTile->SetTileVisionState(ETileVisionState::Visible);
}

void URoomManagerSubsystem::UpdateEnemyMoveVision(ATile* PreviousTile, ATile* CurrentTile) const
{
	// 직전 타일이 플레이어에 의해 Visible이 된 상태가 아니라면 Explored로 변경합니다.
	if (!IsTileVisibleByPlayer(PreviousTile) && PreviousTile->GetTileVisionState() == ETileVisionState::Visible)
	{
		PreviousTile->SetTileVisionState(ETileVisionState::Explored);
	}

	// 새로 밟게 된 타일은 Visible 상태로 변경합니다.
	CurrentTile->SetTileVisionState(ETileVisionState::Visible);
}

void URoomManagerSubsystem::UpdatePlayerRoomState(const ATile* OldTile, const ATile* NewTile)
{
	int32 OldRoomId = INDEX_NONE;
	int32 NewRoomId = INDEX_NONE;
	if (OldTile)
	{
		OldRoomId = OldTile->GetRoomId();
	}
	if (NewTile)
	{
		NewRoomId = NewTile->GetRoomId();
	}

	// 동일한 Room에서 움직인 경우 얼리리턴합니다.
	if (OldRoomId == NewRoomId)
	{
		return;
	}

	// 다른 Room으로 이동한 경우 들어오는 분기입니다.
	FRoomData* OldRoomData = GetMutableRoomData(OldRoomId);
	FRoomData* NewRoomData = GetMutableRoomData(NewRoomId);
	if (OldRoomData)
	{
		--OldRoomData->PlayerCharacterCount;
		if (OldRoomData->PlayerCharacterCount <= 0)
		{
			ChangeTileVisionState(OldRoomId, OldRoomData, ETileVisionState::Explored);
		}
	}
	if (NewRoomData)
	{
		if (NewRoomData->PlayerCharacterCount <= 0)
		{
			ChangeTileVisionState(NewRoomId, NewRoomData, ETileVisionState::Visible);
		}
		++NewRoomData->PlayerCharacterCount;
	}
}

void URoomManagerSubsystem::ChangeTileVisionState(const int32 InRoomId, FRoomData* RoomData, const ETileVisionState VisionState) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!RoomData || !TileManagerSubsystem)
	{
		return;
	}

	// 해당 Room에 소속된 모든 타일의 TileVisionState를 변경합니다.
	for (const auto& RoomTile : RoomData->RoomTiles)
	{
		if (RoomTile.IsValid())
		{
			RoomTile->SetTileVisionState(VisionState);
			if (AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(RoomTile.Get()))
			{
				if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(ActorOnTile))
				{
					CombatInterface->UpdateHiddenByTile(RoomTile.Get());
				}
			}
		}
	}

	if (VisionState == ETileVisionState::Visible)
	{
		// 방문한 경우 입구 타일까지 방문 처리합니다.
		for (const auto& EntranceTile : RoomData->VisibleEntranceTiles)
		{
			if (EntranceTile.IsValid())
			{
				EntranceTile->SetTileVisionState(ETileVisionState::Visible);
				if (AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(EntranceTile.Get()))
				{
					if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(ActorOnTile))
					{
						CombatInterface->UpdateHiddenByTile(EntranceTile.Get());
					}
				}
			}
		}
	}
	else
	{
		// Room에서 빠져나간 경우, 해당 Room의 타일 중 다른 캐릭터가 방문 중인 상태인 Room의 EntranceTile은 다시 시야를 확보합니다.
		for (const auto& Pair : RoomDataMap)
		{
			if (Pair.Key == InRoomId)
			{
				continue;
			}

			for (const auto& EntranceTile : Pair.Value.VisibleEntranceTiles)
			{
				// 현재 플레이어 캐릭터가 아무도 없는 Room이라면 시야를 재확보하지 않습니다.
				if (Pair.Value.PlayerCharacterCount <= 0)
				{
					continue;
				}
				
				if (EntranceTile.IsValid() && EntranceTile->GetRoomId() == InRoomId)
				{
					EntranceTile->SetTileVisionState(ETileVisionState::Explored);
					if (AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(EntranceTile.Get()))
					{
						if (ICombatInterface* CombatInterface = Cast<ICombatInterface>(ActorOnTile))
						{
							CombatInterface->UpdateHiddenByTile(EntranceTile.Get());
						}
					}
				}
			}
		}
	}
}

bool URoomManagerSubsystem::IsTileVisibleByPlayer(const ATile* InTile) const
{
	if (!InTile)
	{
		return false;
	}

	// 타일에 해당하는 RoomData를 가져옵니다.
	const int32 CheckingRoomId = InTile->GetRoomId();
	if (const FRoomData* RoomData = GetRoomData(CheckingRoomId))
	{
		// 해당 Room 안에 플레이어 캐릭터가 1명이라도 있다면 true를 반환합니다.
		if (RoomData->PlayerCharacterCount > 0)
		{
			return true;
		}
	}

	// 플레이어가 입장해있는 Room의 EntranceTile 중 InTile이 존재하는지 확인합니다.
	for (const auto& Pair : RoomDataMap)
	{
		if (Pair.Key == CheckingRoomId)
		{
			continue;
		}
		if (Pair.Value.PlayerCharacterCount <= 0)
		{
			continue;
		}
		
		if (Pair.Value.VisibleEntranceTiles.Contains(InTile))
		{
			return true;
		}
	}
	
	return false;
}

FRoomData* URoomManagerSubsystem::GetMutableRoomData(const int32 RoomId)
{
	return RoomDataMap.Find(RoomId);
}

const FRoomData* URoomManagerSubsystem::GetRoomData(const int32 RoomId) const
{
	return RoomDataMap.Find(RoomId);
}
