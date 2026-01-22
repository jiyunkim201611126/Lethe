// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/** Tile 시스템에서 사용되는 Enum 모음
 * 
 */
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

