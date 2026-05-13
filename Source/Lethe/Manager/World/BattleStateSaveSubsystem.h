// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/SaveGame/BattleStateSaveGame.h"
#include "Subsystems/WorldSubsystem.h"
#include "BattleStateSaveSubsystem.generated.h"

class APlayerCharacterBase;

USTRUCT()
struct FBattleStateSaveContext
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<APlayerCharacterBase*> PlayerCharacters;
};

UCLASS(Config = Game)
class LETHE_API UBattleStateSaveSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void SaveBattleState(const FBattleStateSaveContext& Context);
	void LoadBattleState(const FBattleStateSaveContext& Context);

private:
	UPROPERTY(Config)
	TSubclassOf<UBattleStateSaveGame> BattleStateSaveGameClass;

	const FString SlotName = TEXT("BattleStateSaveSlot");
};
