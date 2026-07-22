// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TargetTileSelectData.generated.h"

class ATile;

USTRUCT()
struct FTargetingIntent
{
	GENERATED_BODY()

	UPROPERTY()
	ATile* HitTile = nullptr;

	UPROPERTY()
	FVector ImpactPoint = FVector::ZeroVector;
};

/** TargetTileSelector의 동작 결과 데이터입니다. */
USTRUCT()
struct FSelectedTarget
{
	GENERATED_BODY()

	UPROPERTY()
	TWeakObjectPtr<ATile> TargetTile = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AActor> ActorOnTile = nullptr;
};

USTRUCT()
struct FTargetSelectResult
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag TargetGroupTag;

	UPROPERTY()
	TArray<FSelectedTarget> Targets;
};