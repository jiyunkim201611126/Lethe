// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TargetSelectionData.generated.h"

class ATile;

/**
 * 플레이어가 카드 사용을 위해 마우스를 클릭한 경우 라인트레이스를 수행, HitResult에서 가져오는 정보입니다.
 * TargetTileSelector가 대상을 수집하기 위해 사용합니다.
 * AIController도 이를 활용하며, 이 경우 대상으로 찍은 플레이어 캐릭터가 서있는 타일이 할당됩니다.
 */
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
struct FTargetSelectionResult
{
	GENERATED_BODY()

	UPROPERTY()
	FGameplayTag TargetGroupTag;

	UPROPERTY()
	TArray<FSelectedTarget> Targets;

	TArray<ATile*> GetTargetTiles() const
	{
		TArray<ATile*> Result;
		for (const FSelectedTarget& Target : Targets)
		{
			if (Target.TargetTile.IsValid())
			{
				Result.Add(Target.TargetTile.Get());
			}
		}
		return Result;
	}

	TArray<AActor*> GetTargetActors() const
	{
		TArray<AActor*> Result;
		for (const FSelectedTarget& Target : Targets)
		{
			if (Target.ActorOnTile.IsValid())
			{
				Result.Add(Target.ActorOnTile.Get());
			}
		}
		return Result;
	}
};