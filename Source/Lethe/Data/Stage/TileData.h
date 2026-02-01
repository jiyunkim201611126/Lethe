// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TileData.generated.h"

class ATile;

/**
 * Tile 시스템에서 사용되는 Enum 모음
 */
UENUM()
enum class EBFSType : uint8
{
	Connection,
	Through,
};

USTRUCT()
struct FAbilityRange
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	EBFSType BFSType;
	
	UPROPERTY(EditDefaultsOnly)
	int32 Depth;
};

UENUM()
enum class ETileMeshType : uint8
{
	Main,
	Main_Under,
	Side,
	Side_Under1,
	Side_Upper,
	Side_Under2,
	Side_Lower,
	Side_Half,
	Side_Under3,
	Side_Under4,
};

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

/**
 * 타일 정보를 담은 구조체
 */
USTRUCT()
struct FTileData
{
	GENERATED_BODY()

	UPROPERTY()
	int32 Floor = 1; // 층 수, 기본은 1층
	
	bool bConnections[6] = {true, true, true, true, true, true};
	int32 RoomID = 0; //ID값은 일단 넣었지만, Room이라는 구조체가 필요한지 잘 모르겠어서 생성시점 외에 사용되진 않음
	
	// 타일 생성 시 동적으로 할당되는 포인터입니다.
	UPROPERTY()
	TWeakObjectPtr<ATile> TileActor;
};