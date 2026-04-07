// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TileData.generated.h"

class ATile;

UENUM(BlueprintType)
enum class EBFSType : uint8
{
	Connection,
	Through,
};

USTRUCT(BlueprintType)
struct FBFSRange
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EBFSType BFSType = EBFSType::Through;

	/** BFS 시 Depth와 같은 뜻으로 사용됩니다. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Distance = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 FloorGap = 1;
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

/**
 * 타일 정보를 담은 구조체
 */
USTRUCT()
struct FTileData
{
	GENERATED_BODY()

	/** 층 수, 기본은 1층 */
	int32 Floor = 1;

	/** true면 Connected와 VerticalConnected 중 하나인 상태입니다. */
	TStaticArray<bool, 6> Connections = {true, true, true, true, true, true};
	
	/** ID값은 일단 넣었지만, Room이라는 구조체가 필요한지 잘 모르겠어서 생성시점 외에 사용되진 않음 */
	int32 RoomId = 0;
	
	/** 타일 생성 시 동적으로 할당되는 포인터입니다. */
	TWeakObjectPtr<ATile> TileActor;
};