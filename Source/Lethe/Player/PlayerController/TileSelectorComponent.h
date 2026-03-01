// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TileSelectorComponent.generated.h"

class ATile;
class IHighlightInterface;
struct FCubeCoord;
struct FAbilityRange;

// 로컬 변수로만 활용하기 때문에 멤버 변수를 Raw 포인터로 사용합니다.
USTRUCT()
struct FTileAndActor
{
	GENERATED_BODY()

	UPROPERTY()
	ATile* Tile = nullptr;

	UPROPERTY()
	AActor* Actor = nullptr;
};

DECLARE_DELEGATE_TwoParams(FOnDetectedOtherTile, const AActor*, const AActor*);

/**
 * 타일 선택과 하이타이팅을 담당하는 클래스입니다.
 */
UCLASS()
class LETHE_API UTileSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTileSelectorComponent();

	void HighlightTileByMouse(AActor* Tile);
	void UnhighlightTileByMouse();
	void HighlightTileByAbility(const TArray<ATile*>& Tiles, const AActor* AbilityOwner);
	void UnhighlightTileByAbility();

	void GetTileAndActorUnderCursor(FTileAndActor& TileAndActor) const;
	AActor* GetActorOnTileUnderCursor() const;
	bool TryGetTilesByDepth(TArray<ATile*>& OutTiles, const AActor* ActorOnTile, const FAbilityRange& InRange) const;

public:
	FOnDetectedOtherTile OnDetectedOtherTile;

private:
	TScriptInterface<IHighlightInterface> LastMouseHoveredTile;
	TScriptInterface<IHighlightInterface> CurrentMouseHoveredTile;

	/**
	 * 현재 하이라이팅된 타일을 추적하는 변수입니다.
	 * 3 * Depth * (Depth + 1) + 1 개만큼의 배열 길이를 가지므로, Depth에 2차 함수로 비례해서 메모리를 잡아먹게 됩니다.
	 * 그러나 메모리 개선을 하기 위해선 이전에 선택됐던 카드의 범위를 다시 한 번 탐색해 Unhighlight를 수행하는 과정이 필요합니다.
	 * CPU 부담을 키우지 않기 위해 메모리 사용을 감수합니다.
	 */
	TArray<TScriptInterface<IHighlightInterface>> CurrentHighlightedTilesByAbility;
};
