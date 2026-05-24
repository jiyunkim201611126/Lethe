// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorSelectorComponent.generated.h"

class ATile;
class IHighlightInterface;
struct FCubeCoord;
struct FBFSRange;

/**
 * 로컬 변수로만 활용하기 때문에 멤버 변수를 Raw 포인터로 사용합니다.
 * Tile은 null인 경우가 거의 없으나, Actor는 해당 Tile 위에 있는 객체기 때문에 null인 경우가 비일비재한 걸 유념하며 사용합니다.
 */ 
USTRUCT()
struct FTileAndActor
{
	GENERATED_BODY()

	UPROPERTY()
	ATile* Tile = nullptr;

	UPROPERTY()
	AActor* Actor = nullptr;
};

enum class ETileRangeQueryType : uint8
{
	Any,
	PlayerMove,
};

DECLARE_DELEGATE_OneParam(FOnDetectedOtherTile, const TArray<AActor*>&);

/**
 * 타일, 캐릭터 선택과 하이라이팅을 담당하는 클래스입니다.
 */
UCLASS()
class LETHE_API UActorSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActorSelectorComponent();

	/**
	 * 캐릭터 선택 중, 혹은 카드 선택 후 마우스로 하이라이팅합니다.
	 * 매개변수로 들어오는 Actor는 캐릭터와 타일이 뒤섞이지 않으며, 반드시 한 종류의 액터만 들어옵니다.
	 */
	void HighlightActorByMouse(const TArray<AActor*>& Actors, const bool bTransparent);
	void UnhighlightActorByMouse();
	void HighlightActorsByAbility(const TArray<ATile*>& Tiles, AActor* AbilityOwner);
	void UnhighlightActorsByAbility();
	
	void GetTileAndActorUnderCursor(FTileAndActor& TileAndActor) const;
	bool TryGetTilesByRangeFromTile(const ATile* Tile, const FBFSRange& InRange, const ETileRangeQueryType QueryType, TArray<ATile*>& OutTiles) const;
	bool TryGetTilesByRangeFromActor(const AActor* ActorOnTile, const FBFSRange& InRange, const ETileRangeQueryType QueryType, TArray<ATile*>& OutTiles) const;

public:
	FOnDetectedOtherTile OnDetectedOtherTile;

private:
	TArray<TScriptInterface<IHighlightInterface>> LastMouseHoveredActors;
	TArray<TScriptInterface<IHighlightInterface>> CurrentMouseHoveredActors;

	/**
	 * 현재 하이라이팅된 타일과 캐릭터를 추적하는 변수입니다.
	 * 3 * Depth * (Depth + 1) + 1 개만큼의 배열 길이를 가지므로, Depth에 2차 함수로 비례해서 메모리를 잡아먹게 됩니다.
	 * 그러나 메모리 개선을 하기 위해선 이전에 선택됐던 카드의 범위를 다시 한 번 탐색해 Unhighlight를 수행하는 과정이 필요합니다.
	 * CPU 부담을 키우지 않기 위해 메모리 사용을 감수합니다.
	 */
	TArray<TScriptInterface<IHighlightInterface>> CurrentHighlightedTilesByAbility;
	TScriptInterface<IHighlightInterface> CurrentHighlightedCharacterByAbility;
};
