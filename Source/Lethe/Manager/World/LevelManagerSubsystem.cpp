// Copyright JETBLU, Inc. All Rights Reserved.

#include "LevelManagerSubsystem.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/Data/LevelData.h"

void ULevelManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (LevelDataPath.IsValid())
	{
		LevelData = Cast<ULevelData>(LevelDataPath.TryLoad());
	}
}

void ULevelManagerSubsystem::ChangeMap(const ELevelType TargetLevelType, const FString& StartTag)
{
	if (CurrentLevelType == TargetLevelType)
	{
		return;
	}

	bIsTransitioning = true;
	NextLevelType = TargetLevelType;
	NextLevelStartTag = StartTag;
	NextLevel = LevelData->GetLevelAssetByType(NextLevelType);

	checkf(!NextLevel.IsNull(), TEXT("NextLevel을 찾을 수 없습니다."));

	// TODO: 로딩 위젯 켜기

	// 로딩 맵으로 이동을 시작합니다.
	const TSoftObjectPtr<UWorld>& LoadingLevel = LevelData->GetLevelAssetByType(ELevelType::Loading);
	checkf(!LoadingLevel.IsNull(), TEXT("LoadingLevel을 찾을 수 없습니다."));
	
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadLoadingMapWithWorld);
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, LoadingLevel);
}

void ULevelManagerSubsystem::OnPostLoadLoadingMapWithWorld(UWorld* World)
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// 이전 레벨에 있던 UObject들이 메모리에서 제대로 내려갈 수 있도록 약간의 딜레이를 주어 이동합니다.
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &ThisClass::DelayedOpenLevel);
	World->GetTimerManager().SetTimer(OpenLevelDelayTimer, TimerDelegate, OpenLevelDelayTime, false);
}

void ULevelManagerSubsystem::DelayedOpenLevel()
{
	PreviousLevelType = CurrentLevelType;
	CurrentLevelType = NextLevelType;

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &ThisClass::OnPostLoadMapWithWorld);
	UGameplayStatics::OpenLevelBySoftObjectPtr(this, NextLevel, true, NextLevelStartTag);
}

void ULevelManagerSubsystem::OnPostLoadMapWithWorld(UWorld* World)
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// 이동을 마쳤으므로 TSoftObjectPtr가 가리키는 경로를 초기화합니다.
	NextLevel.Reset();
	bIsTransitioning = false;

	// TODO: 로딩 위젯 끄기
}
