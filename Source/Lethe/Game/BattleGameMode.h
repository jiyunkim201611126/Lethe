// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameMode.h"
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
	TSubclassOf<ALetheCharacterBase> TestEnemy;
};
