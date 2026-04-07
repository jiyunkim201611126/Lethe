// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameMode.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "BattleGameMode.generated.h"

class ALetheCharacterBase;
class UCharacterDefinitionData;

UCLASS()
class LETHE_API ABattleGameMode : public ALetheGameMode
{
	GENERATED_BODY()

public:
	virtual void RestartPlayer(AController* NewPlayer) override;

private:
	void OnCharacterDefinitionDataLoaded(const TArray<UCharacterDefinitionData*>& CharacterDefinitionDatas) const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<ALetheCharacterBase> TestEnemyClass;

	/**
	 * 기존 드럽게 Index가지고 계산하던 코드 버리고 그냥 5개 하드코딩으로 박았습니다.
	 * 이게 훨씬 의도가 명확하고 바로 파악 가능한 코드입니다.
	 * 추후 Room마다 하나씩 스폰하거나, 좌표를 랜덤하게 찍는 함수 정도만 구현하면 될 것 같습니다.
	 */
	UPROPERTY(EditDefaultsOnly)
	TArray<FCubeCoord> EnemySpawnCoords =
	{
		FCubeCoord(3, 0, -3),
		FCubeCoord(2, 1, -3),
		FCubeCoord(3, -1, -2),
		FCubeCoord(1, 2, -3),
		FCubeCoord(3, -2, -1),
	};
};
