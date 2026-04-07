// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "StageInitData.h"
#include "StageData.generated.h"

enum class ETileMeshType : uint8;

/**
 * 스테이지 데이터를 담은 구조체
 */
USTRUCT()
struct FStageData : public FTableRowBase
{
	GENERATED_BODY()

	/** 타일 블루프린트 클래스 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> TileBP;

	/** 해당 테마에 맞는 타일 이미지 에셋 모음입니다. */
	UPROPERTY(EditDefaultsOnly)
	TMap<ETileMeshType, TSoftObjectPtr<UStaticMesh>> TileMeshes;

	/** 절차적 생성 알고리즘을 위한 데이터만 모아놓은 데이터 에셋입니다. */
	UPROPERTY(EditDefaultsOnly)
	TSoftObjectPtr<UStageInitData> StageInitData;
};
