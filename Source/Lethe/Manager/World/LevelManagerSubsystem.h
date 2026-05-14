// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LevelManagerSubsystem.generated.h"

class ULevelData;

UENUM()
enum class ELevelType : uint8
{
	None = 0,
	Loading,
	DeckEditing,
	Battle,
};

DECLARE_MULTICAST_DELEGATE(FOnStartLevelChanged);

/**
 * 레벨 이동을 담당하는 Subsystem입니다.
 */
UCLASS(Config = Game)
class LETHE_API ULevelManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	//~ End of USubsystem Interface
	
	void StartLevelTransition(const ELevelType TargetLevelType, const FString& InOptionString);

	ELevelType GetCurrentLevelType() const;

private:
	void OnPostLoadLoadingMapWithWorld(UWorld* World);
	void DelayedOpenLevel();
	void OnPostLoadMapWithWorld(UWorld* World);

public:
	FOnStartLevelChanged OnStartLevelChanged;

private:
	/** TODO: 임시로 CurrentLevelType에 DeckEditing을 할당해두었습니다. 추후 MainMenu 같은 레벨이 생기면 해당 enum 선언 후 할당해줍니다. */
	ELevelType PreviousLevelType = ELevelType::None;
	ELevelType CurrentLevelType = ELevelType::DeckEditing;
	ELevelType NextLevelType = ELevelType::None;

	TSoftObjectPtr<UWorld> NextLevel;
	FString OptionString;

	UPROPERTY(Config)
	FSoftObjectPath LevelDataPath;

	UPROPERTY()
	TObjectPtr<ULevelData> LevelData;

	FTimerHandle OpenLevelDelayTimer;
	const float OpenLevelDelayTime = 1.0f;

	uint8 bIsTransitioning : 1 = false;
};
