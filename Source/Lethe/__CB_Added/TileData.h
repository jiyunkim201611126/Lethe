// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CubeCoord.h"
#include "TileData.generated.h"

UENUM()
enum class ETileDirection : uint8
{
	LeftTop,
	Left,
	LeftBottom,
	RightBottom,
	Right,
	RightTop,
};

/** 타일 정보를 담은 구조체
 * 
 */
USTRUCT()
struct FTileData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Floor = 1; // 층 수, 기본은 1층
	
	bool bConnections[6] = {true, true, true, true, true, true};
	int32 RoomID = 0; //ID값은 일단 넣었지만, Room이라는 구조체가 필요한지 잘 모르겠어서 생성시점 외에 사용되진 않음
};