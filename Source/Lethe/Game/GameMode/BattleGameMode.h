// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameMode.h"
#include "Lethe/Data/Stage/StageData.h"
#include "BattleGameMode.generated.h"

class ALetheCharacterBase;
class UCharacterDefinitionData;
class UPrimaryDataAsset;

UCLASS()
class LETHE_API ABattleGameMode : public ALetheGameMode
{
	GENERATED_BODY()

public:
	virtual void RestartPlayer(AController* NewPlayer) override;

	void OnFloorTransitionStarted() const;

	AController* GetController() const;

private:
	void OnCharacterDefinitionDataLoaded(const TArray<UPrimaryDataAsset*>& CharacterDefinitions);

	void InitRoomRoles(const TArray<UPrimaryDataAsset*>& CharacterDefinitions = {}) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	EStageType StageType = EStageType::Forest;
	
	/** 테스트를 위해 시작하자마자 전투에 즉시 돌입하기 편리하도록 선언된 변수입니다. */
	UPROPERTY(EditDefaultsOnly)
	bool bSpawnEnemyNearly = false;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ALetheCharacterBase> TestEnemyClass;

private:
	TWeakObjectPtr<AController> Controller;
};
