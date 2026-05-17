// Copyright JETBLU, Inc. All Rights Reserved.

#include "RoomManagerSubsystem.h"

#include "TileManagerSubsystem.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/Stage/RoomRoleAssignmentRuleData.h"
#include "Lethe/Interface/TileVisionAffectedInterface.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"

void URoomManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();
	
	Clear();
}

void URoomManagerSubsystem::Clear()
{
	RoomDataMap.Empty();
	RoleAssignedRoomIds.Empty();
}

void URoomManagerSubsystem::SetRoomData(TMap<int32, FRoomData>&& InRoomData)
{
	Clear();
	RoomDataMap = MoveTemp(InRoomData);
}

void URoomManagerSubsystem::NotifyActorTileChanged(const AActor* InActor, const ATile* OldTile, const ATile* NewTile)
{
	if (!InActor || !NewTile)
	{
		// OldTile은 nullptr일 수 있습니다.
		return;
	}

	if (InActor->Implements<UPlayerCharacterInterface>())
	{
		UpdatePlayerRoomState(OldTile, NewTile);
	}
}

void URoomManagerSubsystem::RevealEnemyTile(const ATile* InTile) const
{
	SetTileStackVisionState(InTile, ETileVisionState::Visible);
}

void URoomManagerSubsystem::UpdateEnemyMoveVision(const ATile* OldTile, const ATile* NewTile) const
{
	if (!OldTile || !NewTile)
	{
		return;
	}
	
	// 직전 타일이 플레이어에 의해 Visible이 된 상태가 아니라면 Explored로 변경합니다.
	if (!IsTileVisibleByPlayer(OldTile) && OldTile->GetTileVisionState() == ETileVisionState::Visible)
	{
		SetTileStackVisionState(OldTile, ETileVisionState::Explored);
	}

	// 새로 밟게 된 타일은 Visible 상태로 변경합니다.
	SetTileStackVisionState(NewTile, ETileVisionState::Visible);
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
			SetRoomVisionState(OldRoomId, OldRoomData, ETileVisionState::Explored);
		}
	}
	if (NewRoomData)
	{
		if (NewRoomData->PlayerCharacterCount <= 0)
		{
			SetRoomVisionState(NewRoomId, NewRoomData, ETileVisionState::Visible);
		}
		++NewRoomData->PlayerCharacterCount;
	}
}

void URoomManagerSubsystem::SetTileStackVisionState(const ATile* InTile, const ETileVisionState VisionState) const
{
	if (!InTile)
	{
		return;
	}
	
	if (const FRoomData* RoomData = GetRoomData(InTile->GetRoomId()))
	{
		const FCubeCoord TileCoord = InTile->GetCubeCoord();
		for (const auto& RoomTile : RoomData->RoomTiles)
		{
			if (RoomTile.IsValid() && RoomTile->GetCubeCoord() == TileCoord)
			{
				RoomTile->SetTileVisionState(VisionState);
			}
		}
	}
}

void URoomManagerSubsystem::SetRoomVisionState(const int32 InRoomId, FRoomData* RoomData, const ETileVisionState VisionState) const
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
				if (ITileVisionAffectedInterface* TileVisionAffectedInterface = Cast<ITileVisionAffectedInterface>(ActorOnTile))
				{
					TileVisionAffectedInterface->UpdateHiddenByTile(RoomTile.Get());
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
					if (ITileVisionAffectedInterface* TileVisionAffectedInterface = Cast<ITileVisionAffectedInterface>(ActorOnTile))
					{
						TileVisionAffectedInterface->UpdateHiddenByTile(EntranceTile.Get());
					}
				}
			}
		}
	}
	else
	{
		// Room에서 빠져나간 경우, 해당 Room의 타일 중 다른 캐릭터가 방문 중인 상태인 Room의 EntranceTile은 다시 시야를 재확보합니다.
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
					EntranceTile->SetTileVisionState(ETileVisionState::Visible);
					if (AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(EntranceTile.Get()))
					{
						if (ITileVisionAffectedInterface* TileVisionAffectedInterface = Cast<ITileVisionAffectedInterface>(ActorOnTile))
						{
							TileVisionAffectedInterface->UpdateHiddenByTile(EntranceTile.Get());
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

bool URoomManagerSubsystem::TryFindRoomRoleCandidates(const URoomRoleAssignmentRuleData* RoomRoleAssignmentRuleData, TArray<FRoomRolePlacementCandidate>& OutCandidates) const
{
	OutCandidates.Reset();
	
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!RoomRoleAssignmentRuleData || !TileManagerSubsystem)
	{
		return false;
	}
	
	for (const auto& Pair : RoomDataMap)
	{
		// 이미 Role이 부여된 Room은 스킵합니다.
		if (RoleAssignedRoomIds.Contains(Pair.Key))
		{
			continue;
		}

		// 등차수열 합으로 Distance에 해당하는 정육각 타일 개수를 계산합니다.
		const int32 Distance = RoomRoleAssignmentRuleData->RequiredSpaceRangeDistance;
		const int32 RequiredCoordCount = 1 + 3 * Distance * (Distance + 1);

		// RoomSize 자체가 해당 개수보다 적은 경우 스킵합니다.
		if (Pair.Value.RoomSize < RequiredCoordCount)
		{
			continue;
		}

		// 후보가 될 수 있는 Room 내 모든 타일을 순회합니다.
		for (const auto& RoomTile : Pair.Value.RoomTiles)
		{
			if (RoomTile.IsValid() && RoomTile->IsTopTile())
			{
				TSet<FCubeCoord> TempCoords;
				TileManagerSubsystem->TileBFS(RoomTile->GetCubeCoord(), Distance, RoomRoleAssignmentRuleData->RequiredSpaceRangeBFSType, TempCoords,
					[](const FTileData* CurrentTileData, const FTileData* NextTileData)
					{
						// 같은 Room 내의 좌표만 순회합니다.
						if (CurrentTileData && NextTileData)
						{
							return CurrentTileData->RoomId == NextTileData->RoomId;
						}
						return false;
					},
					[](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
					{
						// 일단 모든 좌표를 선택합니다.
						return true;
					});

				// 정육각 형태를 만족한 경우 들어가는 분기입니다.
				if (RequiredCoordCount <= TempCoords.Num())
				{
					// CoordsSlots를 복사해서 가져온 후, 모든 좌표에 현재 중심 타일 좌표를 더해 월드 좌표로 변환합니다.
					TArray<FRoomCoordSlot> CoordSlots = RoomRoleAssignmentRuleData->CoordSlots;
					bool bValidCoordSlots = true;
					
					for (FRoomCoordSlot& CoordSlot : CoordSlots)
					{
						FCubeCoord WorldCoord = CoordSlot.SlotCoord + RoomTile->GetCubeCoord();
						if (!TempCoords.Contains(WorldCoord))
						{
							LETHE_LOG(LogRoomManager, Error, "%s Room Role의 %s Slot Type의 좌표가 범위를 벗어난 상태로 작성되었습니다.", *LogHelper::EnumToString(RoomRoleAssignmentRuleData->RoomRole), *LogHelper::EnumToString(CoordSlot.SlotType))
							bValidCoordSlots = false;
							continue;
						}
						CoordSlot.SlotCoord = WorldCoord;
					}

					if (bValidCoordSlots)
					{
						FRoomRolePlacementCandidate& Candidate = OutCandidates.AddDefaulted_GetRef();
						Candidate.RoomId = Pair.Key;
						Candidate.RoomSize = Pair.Value.RoomSize;
						Candidate.CenterCoord = Pair.Value.CenterCoord;
						Candidate.CoordSlots = MoveTemp(CoordSlots);
					}
				}
			}
		}
	}
	return !OutCandidates.IsEmpty();
}

bool URoomManagerSubsystem::TryGetDistantRoomIds(const int32 StartRoomId, TArray<int32>& OutRoomIds) const
{
	OutRoomIds.Reset();
	OutRoomIds.Reserve(GetRoomCount() - 1);

	TSet<int32> VisitedRoomIds;
	TQueue<int32> Queue;
	VisitedRoomIds.Add(StartRoomId);
	Queue.Enqueue(StartRoomId);

	// 입구 타일을 통해서 Room이라는 영역을 기준으로 몇 칸 떨어진 Room인지를 파악, 이를 BFS로 순회하며 기록합니다.
	while (!Queue.IsEmpty())
	{
		int32 CurrentRoomId = INDEX_NONE;
		Queue.Dequeue(CurrentRoomId);

		const FRoomData* CurrentRoomData = GetRoomData(CurrentRoomId);
		if (!CurrentRoomData)
		{
			continue;
		}
		
		for (const auto& EntranceTile : CurrentRoomData->VisibleEntranceTiles)
		{
			if (!EntranceTile.IsValid())
			{
				continue;
			}
			
			const int32 NextRoomId = EntranceTile->GetRoomId();
			if (!VisitedRoomIds.Contains(NextRoomId))
			{
				// 아직 방문하지 않은 Room이라면 이를 기록하고, 다음 방문을 위해 Queue에 추가합니다.
				VisitedRoomIds.Add(NextRoomId);
				Queue.Enqueue(NextRoomId);

				if (!RoleAssignedRoomIds.Contains(NextRoomId))
				{
					// Role이 부여되지 않은 Room의 Id만 추가합니다.
					OutRoomIds.Add(NextRoomId);
				}
			}
		}
	}

	return !OutRoomIds.IsEmpty();
}

void URoomManagerSubsystem::MarkRoomRoleAssigned(const FRoomRolePlacementCandidate& Candidate)
{
	if (Candidate.RoomId != INDEX_NONE)
	{
		RoleAssignedRoomIds.Add(Candidate.RoomId);
	}
}

FRoomData* URoomManagerSubsystem::GetMutableRoomData(const int32 RoomId)
{
	return RoomDataMap.Find(RoomId);
}

const FRoomData* URoomManagerSubsystem::GetRoomData(const int32 RoomId) const
{
	return RoomDataMap.Find(RoomId);
}

int32 URoomManagerSubsystem::GetRoomCount() const
{
	return RoomDataMap.Num();
}
