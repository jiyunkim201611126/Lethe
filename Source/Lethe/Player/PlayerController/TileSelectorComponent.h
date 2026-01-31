// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TileSelectorComponent.generated.h"

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

	AActor* TryGetActorOnTileUnderCursor() const;

private:
	TScriptInterface<IHighlightInterface> LastHoveredTile;
	TScriptInterface<IHighlightInterface> CurrentHoveredTile;
};
