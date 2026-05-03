// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.h"
#include "RoomData.generated.h"

class ATile;

USTRUCT()
struct FRoomData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 RoomSize = 0;
	
	UPROPERTY()
	FCubeCoord CenterCoord;

	/** 타일 생성 시 동적으로 할당되는 포인터 배열입니다. */
	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> RoomTiles;

	/** 해당 Room에 소속되지 않는, 바깥 입구 타일의 좌표입니다. */
	UPROPERTY()
	TArray<FCubeCoord> VisibleEntranceCoords;

	/** 해당 Room에 소속되지 않는, 시야 확보 시 사용되는 바깥 입구 타일입니다. */
	UPROPERTY()
	TArray<TWeakObjectPtr<ATile>> VisibleEntranceTiles;

	/** 현재 Room 내부에 있는 플레이어 캐릭터 수입니다. */
	UPROPERTY()
	int32 PlayerCharacterCount = 0;
};
