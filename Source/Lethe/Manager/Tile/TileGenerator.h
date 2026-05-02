// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Data/Stage/RoomData.h"
#include "Lethe/Data/Stage/TileData.h"

class UStageInitData;
class UWorld;
struct FStageData;

struct FTileGenerationResult
{
	TMap<FCubeCoord, FTileData> TileDataMap;
	TMap<int32, FRoomData> RoomDataMap;
};

class LETHE_API FTileGenerator
{
public:
	static bool GenerateTileMap(UWorld* World, const FStageData* StageData, const UStageInitData* StageInitData, FTileGenerationResult& OutResult);
};
