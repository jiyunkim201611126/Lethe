// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "EffectTargetSelector.generated.h"

class ATile;

UCLASS(Abstract, NotBlueprintable, BlueprintType, EditInlineNew, DefaultToInstanced)
class LETHE_API UEffectTargetSelector : public UObject
{
	GENERATED_BODY()

public:
	virtual void Select(const AActor* AvatarActor, const ATile* CenterTile, TArray<TWeakObjectPtr<ATile>>& OutTiles);
};
