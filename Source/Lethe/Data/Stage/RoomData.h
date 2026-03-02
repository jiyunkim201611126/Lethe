#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.h"
#include "RoomData.generated.h"

USTRUCT()
struct FRoomData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 RoomSize;
	
	UPROPERTY()
	FCubeCoord CenterCoords;
	
	FRoomData() : RoomSize(0), CenterCoords(FCubeCoord(0, 0, 0)) {}
	
	constexpr FRoomData(const int32 InRoomSize, const FCubeCoord InCenterCoords)
		: RoomSize(InRoomSize), CenterCoords(FCubeCoord(InCenterCoords.Q, InCenterCoords.R, InCenterCoords.S)) {}
};
