// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EffectTargetSelector.h"
#include "Lethe/Data/Stage/TileData.h"
#include "BFSTargetSelector.generated.h"

UCLASS()
class LETHE_API UBFSTargetSelector : public UEffectTargetSelector
{
	GENERATED_BODY()

public:
	virtual void Select(const AActor* AvatarActor, const ATile* CenterTile, TArray<TWeakObjectPtr<ATile>>& OutTiles) override;

protected:
	UPROPERTY(EditDefaultsOnly)
	FBFSRange Range;
};
