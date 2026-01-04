// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "LetheAssetManager.generated.h"

UCLASS()
class LETHE_API ULetheAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static ULetheAssetManager& Get();

protected:
	virtual void StartInitialLoading() override;
};
