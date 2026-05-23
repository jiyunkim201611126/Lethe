// Copyright JETBLU, Inc. All Rights Reserved.

#include "RoomManagerSubsystem.h"

#include "TileManagerSubsystem.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Character/EnemyCharacterBase.h"
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
	if (const UWorld* World = GetWorld())
	{
		for (auto& Pair : RoomRevealTimerHandles)
		{
			World->GetTimerManager().ClearTimer(Pair.Value);
		}
	}
	
	RoomDataMap.Empty();
	RoleAssignedRoomIds.Empty();
	PlayerToTile.Empty();
	RecognizableCoords.Empty();
	TemporarilyVisibleRoomIds.Empty();
	RoomRevealTimerHandles.Empty();
	EnemyVisibleCoords.Empty();
	PreviousVisionStates.Empty();
}

void URoomManagerSubsystem::SetRoomData(TMap<int32, FRoomData>&& InRoomData)
{
	Clear();
	RoomDataMap = MoveTemp(InRoomData);
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

void URoomManagerSubsystem::NotifyCharacterTileChanged(AActor* InCharacter, const ATile* OldTile, ATile* NewTile)
{
	if (!InCharacter || !NewTile)
	{
		// OldTile은 nullptr일 수 있습니다.
		return;
	}

	if (InCharacter->Implements<UPlayerCharacterInterface>())
	{
		PlayerToTile.Add(InCharacter, NewTile);
		UpdatePlayerRoomState(OldTile, NewTile);
		RefreshPlayerVision();
	}
	
	if (InCharacter->IsA<AEnemyCharacterBase>())
	{
		if (OldTile)
		{
			EnemyVisibleCoords.Remove(OldTile->GetCubeCoord());
		}
		EnemyVisibleCoords.Add(NewTile->GetCubeCoord());
		RecognizableCoords.Add(NewTile->GetCubeCoord());
		ApplyVisionSnapshot();
	}
}

void URoomManagerSubsystem::RevealEnemyTile(const ATile* InTile)
{
	if (!InTile)
	{
		return;
	}
	
	EnemyVisibleCoords.Add(InTile->GetCubeCoord());
	RecognizableCoords.Add(InTile->GetCubeCoord());
	ApplyVisionSnapshot();
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

	// 다른 Room으로 이동한 경우 Vision에 대한 처리를 시작합니다.
	if (FRoomData* NewRoomData = GetMutableRoomData(NewRoomId))
	{
		// 해당 Room에 대한 첫 입장 시에만 Visible로 변경합니다.
		if (!NewRoomData->bIsVisited)
		{
			NewRoomData->bIsVisited = true;
			TemporarilyVisibleRoomIds.Add(NewRoomId);

			TSet<FCubeCoord> RoomCoords;
			TSet<FCubeCoord> BorderCoords;
			CollectRoomBorderCoords(*NewRoomData, RoomCoords, BorderCoords);
			RecognizableCoords.Append(RoomCoords);
			RecognizableCoords.Append(BorderCoords);

			// 잠시 후 Recognizable로 변경합니다.
			FTimerHandle& TimerHandle = RoomRevealTimerHandles.FindOrAdd(NewRoomId);
			TWeakObjectPtr<URoomManagerSubsystem> WeakThis = MakeWeakObjectPtr(this);
			GetWorld()->GetTimerManager().SetTimer(TimerHandle,
				[WeakThis, NewRoomId]()
				{
					if (WeakThis.IsValid())
					{
						WeakThis->TemporarilyVisibleRoomIds.Remove(NewRoomId);
						WeakThis->RoomRevealTimerHandles.Remove(NewRoomId);
						WeakThis->ApplyVisionSnapshot();
					}
				}, RoomTemporarilyVisibleDuration, false);
		}
	}
}

void URoomManagerSubsystem::RefreshPlayerVision()
{
	for (auto It = PlayerToTile.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid() || !It.Value().IsValid())
		{
			It.RemoveCurrent();
		}
	}
	
	TSet<FCubeCoord> PlayerVisibleCoords;
	CollectPlayerVisibleCoords(PlayerVisibleCoords);
	RecognizableCoords.Append(PlayerVisibleCoords);
	ApplyVisionSnapshot(PlayerVisibleCoords);
}

void URoomManagerSubsystem::CollectPlayerVisibleCoords(TSet<FCubeCoord>& OutCoords) const
{
	OutCoords.Reset();

	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	for (const auto& Pair : PlayerToTile)
	{
		if (!Pair.Key.IsValid() || !Pair.Value.IsValid())
		{
			continue;
		}

		const IPlayerCharacterInterface* PlayerCharacterInterface = Cast<IPlayerCharacterInterface>(Pair.Key);
		if (!PlayerCharacterInterface)
		{
			continue;
		}

		const int32 VisionRange = PlayerCharacterInterface->GetVisionRange();
		if (VisionRange <= 0)
		{
			continue;
		}

		const ATile* PlayerTile = Pair.Value.Get();
		const int32 PlayerFloor = TileManagerSubsystem->GetTileFloor(PlayerTile);
		if (PlayerFloor == INDEX_NONE)
		{
			continue;
		}

		TSet<FCubeCoord> VisibleCoordsFromTile;
		TileManagerSubsystem->TileBFS(PlayerTile->GetCubeCoord(), VisionRange, EBFSType::Through, VisibleCoordsFromTile,
			[PlayerFloor](const FTileData* CurrentTileData, const FTileData* NextTileData)
			{
				if (!CurrentTileData || !NextTileData)
				{
					return false;
				}

				// 플레이어보다 높은 타일은 그 타일까지만 보이고, 그 너머로는 시야가 이어지지 않습니다.
				return CurrentTileData->Floor <= PlayerFloor;
			},
			[VisionRange](const FCubeCoord& CurrentCoord, const FTileData* TileData, const int32 Depth)
			{
				return Depth <= VisionRange;
			});

		OutCoords.Append(VisibleCoordsFromTile);
	}
}

void URoomManagerSubsystem::CollectRoomCoords(const FRoomData& RoomData, TSet<FCubeCoord>& OutCoords) const
{
	for (const auto& RoomTile : RoomData.RoomTiles)
	{
		if (RoomTile.IsValid())
		{
			OutCoords.Add(RoomTile->GetCubeCoord());
		}
	}
}

void URoomManagerSubsystem::CollectRoomBorderCoords(const FRoomData& RoomData, TSet<FCubeCoord>& OutRoomCoords, TSet<FCubeCoord>& OutBorderCoords) const
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	CollectRoomCoords(RoomData, OutRoomCoords);

	for (const FCubeCoord& RoomCoord : OutRoomCoords)
	{
		for (int32 Direction = 0; Direction < 6; ++Direction)
		{
			const FCubeCoord NeighborCoord = RoomCoord + FCubeCoord::GetDirection(Direction);
			if (OutRoomCoords.Contains(NeighborCoord))
			{
				continue;
			}

			if (TileManagerSubsystem->GetTile(NeighborCoord))
			{
				OutBorderCoords.Add(NeighborCoord);
			}
		}
	}
}

void URoomManagerSubsystem::ApplyVisionSnapshot()
{
	TSet<FCubeCoord> PlayerVisibleCoords;
	CollectPlayerVisibleCoords(PlayerVisibleCoords);
	ApplyVisionSnapshot(PlayerVisibleCoords);
}

void URoomManagerSubsystem::ApplyVisionSnapshot(const TSet<FCubeCoord>& PlayerVisibleCoords)
{
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (!TileManagerSubsystem)
	{
		return;
	}

	// 좌표에 해당하는 타일이 이번에 어떤 VisionState가 되어야 하는지 기록합니다.
	TMap<FCubeCoord, ETileVisionState> DesiredVisionStates;

	// Room 입장 직후 상태라면 해당 Room 타일들의 좌표를 가져와 Visible로 기록합니다.
	for (const int32 RoomId : TemporarilyVisibleRoomIds)
	{
		if (const FRoomData* RoomData = GetRoomData(RoomId))
		{
			TSet<FCubeCoord> RoomCoords;
			CollectRoomCoords(*RoomData, RoomCoords);
			for (const FCubeCoord& Coord : RoomCoords)
			{
				DesiredVisionStates.Add(Coord, ETileVisionState::Visible);
			}
		}
	}

	// 플레이어 시야에 들어와있는 좌표를 가져와 Visible로 기록합니다.
	for (const FCubeCoord& Coord : PlayerVisibleCoords)
	{
		DesiredVisionStates.Add(Coord, ETileVisionState::Visible);
	}

	// 전투 중인 적이 서있는 좌표를 가져와 Visible로 기록합니다.
	for (const FCubeCoord& Coord : EnemyVisibleCoords)
	{
		DesiredVisionStates.Add(Coord, ETileVisionState::Visible);
	}

	// 인식 가능한 모든 좌표를 가져와, 아직 기록되지 않은 좌표에만 Recognizable로 기록합니다.
	for (const FCubeCoord& Coord : RecognizableCoords)
	{
		if (!DesiredVisionStates.Contains(Coord))
		{
			DesiredVisionStates.Add(Coord, ETileVisionState::Recognizable);
		}
	}

	for (const auto& Pair : DesiredVisionStates)
	{
		if (const ETileVisionState* PreviousVisionState = PreviousVisionStates.Find(Pair.Key))
		{
			if (*PreviousVisionState == Pair.Value)
			{
				// 해당 좌표의 타일에 Vision 변화가 없다면 스킵합니다.
				continue;
			}
		}

		// 좌표에 해당하는 모든 타일의 Vision을 업데이트합니다.
		ATile* TopTile = TileManagerSubsystem->GetTile(Pair.Key);
		if (!TopTile)
		{
			continue;
		}
		
		if (auto* UnderTiles = TileManagerSubsystem->GetUnderTiles(Pair.Key))
		{
			for (auto& UnderTile : *UnderTiles)
			{
				if (UnderTile.IsValid())
				{
					UnderTile->SetTileVisionState(Pair.Value);
				}
			}
		}

		TopTile->SetTileVisionState(Pair.Value);
		if (AActor* ActorOnTile = TileManagerSubsystem->GetActorOnTile(TopTile))
		{
			if (ActorOnTile->Implements<UTileVisionAffectedInterface>())
			{
				ITileVisionAffectedInterface::Execute_UpdateHiddenByTile(ActorOnTile, TopTile);
			}
		}
	}

	PreviousVisionStates = MoveTemp(DesiredVisionStates);
}

bool URoomManagerSubsystem::IsTileVisibleByPlayer(const ATile* InTile) const
{
	if (!InTile)
	{
		return false;
	}

	TSet<FCubeCoord> PlayerVisibleCoords;
	CollectPlayerVisibleCoords(PlayerVisibleCoords);
	if (PlayerVisibleCoords.Contains(InTile->GetCubeCoord()))
	{
		return true;
	}

	return TemporarilyVisibleRoomIds.Contains(InTile->GetRoomId());
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
