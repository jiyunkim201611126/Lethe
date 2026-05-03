// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.h"
#include "RoomData.generated.h"

class ATile;

enum class ERoomVisionState : uint8
{
	/** 아직 방문하지 않은 Room으로, 렌더링 자체가 꺼진 상태입니다. */
	Hidden,
	/** 현재 방문 중인 Room으로, Room 내 모든 Tile, 그리고 연결된 Room의 입구 타일까지 볼 수 있는 상태입니다. */
	Visible,
	/** 한 번 방문했던 Room으로, 타일 형태는 볼 수 있으나 그 위 적은 볼 수 없는 상태입니다. */
	Explored
};

USTRUCT()
struct FRoomData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 RoomSize = 0;
	
	UPROPERTY()
	FCubeCoord CenterCoords;

	/** 타일 생성 시 동적으로 할당되는 포인터 배열입니다. */
	TArray<TWeakObjectPtr<ATile>> RoomTiles;

	ERoomVisionState Visibility = ERoomVisionState::Hidden;
};
