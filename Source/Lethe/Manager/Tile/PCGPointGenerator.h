// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Data/Stage/TileData.h"

class UPCGPointArrayData;

class LETHE_API PCGPointGenerator
{
public:
	//PCG Point를 생성하는 함수
	static UPCGPointArrayData* GeneratePCGPoint(const TMap<FCubeCoord, FTileData>& TileDataMap, const TMap<FCubeCoord, TArray<FSoftObjectPath>>& TileMeshArray);
	//생성된 Point를 기반으로 DA를 작성하는 함수
	//디버깅 편의성을 위해 에디터 타임에선 이렇게 진행하고, 스탠드 얼론 출시 시에는 PCG 그래프에 직접 주입하는 방향으로 변경
	static void BakePCGDataAsset(UPCGPointArrayData* TilePoints);
};
