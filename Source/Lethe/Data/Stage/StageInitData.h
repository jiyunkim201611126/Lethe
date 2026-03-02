// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StageInitData.generated.h"

/**
 * 절차적 생성 알고리즘을 위한 데이터만 모아놓은 데이터 에셋
 */
UCLASS()
class LETHE_API UStageInitData : public UDataAsset
{
	GENERATED_BODY()

public:
	//맵 시드 번호(0으로 놓을 시, 랜덤 시드를 사용함), 가능하면 0 ~ 9999 값으로
	UPROPERTY(EditDefaultsOnly)
	int32 MapSeed = 0;
	
	//맵 가로 길이(반드시 홀수)
	UPROPERTY(EditDefaultsOnly)
	int32 MapWidth;
	
	//맵 세로 길이(반드시 홀수)
	UPROPERTY(EditDefaultsOnly)
	int32 MapHeight;

	//최대 층 수
	UPROPERTY(EditDefaultsOnly)
	int32 MaxFloor;

	//층 생성의 총 시도 횟수
	UPROPERTY(EditDefaultsOnly)
	int32 FloorIncrementTrialsCount;

	//거리에 따른 타일 생성 확률 커브
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UCurveFloat> ProbabilityCurve;

	//타일 생성 및 실패시, 연속성을 위해 확률 계산을 무시할 타일 수
	UPROPERTY(EditDefaultsOnly)
	int32 ConsecutiveTileCount;

	//서로 다른 층의 타일이 연결될 때, 6경로 기준으로 평균 몇 개나 연결되도록 할 것인지 (6일 경우, 모든 경로가 연결됨)
	UPROPERTY(EditDefaultsOnly)
	int32 AverageConnectionPerSixWays;
};
