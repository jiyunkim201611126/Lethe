// Copyright JETBLU, Inc. All Rights Reserved.

#include "RoomManagerSubsystem.h"

void URoomManagerSubsystem::Deinitialize()
{
	Super::Deinitialize();

	RoomDataMap.Empty();
}

void URoomManagerSubsystem::SetRoomData(TMap<int32, FRoomData>&& InRoomData)
{
	RoomDataMap = MoveTemp(InRoomData);
}

const FRoomData* URoomManagerSubsystem::GetRoomData(const int32 RoomId) const
{
	return RoomDataMap.Find(RoomId);
}
