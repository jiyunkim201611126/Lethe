// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "StageData.h"
#include "LetheGameInstance.generated.h"


UCLASS()
class LETHE_API ULetheGameInstance : public UGameInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, Category = BaseSetting)
		TSoftObjectPtr<UDataTable> StageDataTable;

private:
	int stageIndex = -1;

public:
	const FStageData* GetStageData(const FName& StageName) const;
};
