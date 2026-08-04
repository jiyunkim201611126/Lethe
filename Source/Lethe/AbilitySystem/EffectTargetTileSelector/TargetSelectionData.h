// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TargetSelectionData.generated.h"

class ATile;
class UTileManagerSubsystem;

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

/**
 * CandidateTiles나 TargetTiles가 필요한 경우 CardAbility의 함수를 호출하며 사용하는 구조체입니다.
 * 외부에서는 AvatarActor와 TargetingIntent만 채워서 호출하면 나머지는 CardAbility와 TargetTileSelector가 채워줍니다.
 */
struct FEffectTargetTileSelectorContext
{
	const AActor* AvatarActor = nullptr;
	FTargetingIntent TargetingIntent;

	const UTileManagerSubsystem* TileManagerSubsystem = nullptr;
	const ATile* SourceTile = nullptr;

	TArray<ATile*> OutSelectCandidateTiles;
	
	/** TargetCandidate 계산 결과로, Actor 검증 전의 그룹별 후보입니다. */
	TArray<FTargetSelectionResult> OutTargetCandidates;
	/** TargetCandidate를 Actor/팀 관계 기준으로 검증한 최종 대상입니다. */
	TArray<FTargetSelectionResult> OutTargetResults;

	/** 유효한 타겟이 하나라도 있다면 true로 할당됩니다. */
	bool bHasValidActorTarget = false;

	bool IsValid() const
	{
		return AvatarActor && TileManagerSubsystem && SourceTile && TargetingIntent.HitTile;
	}
};

struct FEffectTargetTileSelectorResult
{
	TArray<ATile*> OutSelectCandidateTiles;
	
	/** TargetCandidate 계산 결과로, Actor 검증 전의 그룹별 후보입니다. */
	TArray<FTargetSelectionResult> OutTargetCandidates;
	/** TargetCandidate를 Actor/팀 관계 기준으로 검증한 최종 대상입니다. */
	TArray<FTargetSelectionResult> OutTargetResults;

	/** 유효한 타겟이 하나라도 있다면 true로 할당됩니다. */
	bool bHasValidActorTarget = false;
};
