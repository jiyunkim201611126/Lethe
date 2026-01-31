// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TileSelectorComponent.generated.h"

class ATile;
struct FCubeCoord;
class IHighlightInterface;

/**
 * 타일 선택과 하이타이팅을 담당하는 클래스입니다.
 */
UCLASS()
class LETHE_API UTileSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTileSelectorComponent();

	void HighlightTile(AActor* Tile);
	void UnhighlightTile();

	AActor* GetActorOnTileUnderCursor() const;
	bool TryGetTilesByDepth(TArray<ATile*>& OutTiles, const FCubeCoord& CenterCoord, const int32 InDepth) const;

private:
	TScriptInterface<IHighlightInterface> LastHoveredTile;
	TScriptInterface<IHighlightInterface> CurrentHoveredTile;
};
