// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ActorSelectorComponent.generated.h"

enum class EHighlightReason : uint8;
class ATile;
class IHighlightInterface;
struct FCubeCoord;
struct FBFSRange;

/**
 * 로컬 변수로만 활용하기 때문에 멤버 변수를 Raw 포인터로 사용합니다.
 * Tile은 null인 경우가 거의 없으나, Actor는 해당 Tile 위에 있는 객체기 때문에 null인 경우가 비일비재한 걸 유념하며 사용합니다.
 */ 
struct FTileHitResult
{
	ATile* Tile = nullptr;
	AActor* ActorOnTile = nullptr;
	FVector ImpactPoint = FVector::ZeroVector;
};

enum class ETileRangeQueryType : uint8
{
	Any,
	PlayerMove,
};

/**
 * 타일, 캐릭터 선택과 하이라이팅을 담당하는 클래스입니다.
 */
UCLASS()
class LETHE_API UActorSelectorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UActorSelectorComponent();
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	bool SetHighlightedActors(const EHighlightReason Reason, const TArray<AActor*>& InActors);
	bool SetHighlightedTiles(const EHighlightReason Reason, const TArray<ATile*>& InTiles);
	void ClearHighlightedActors(const EHighlightReason Reason);
	void ClearAllHighlightedActors();
	
	void GetTileHitResult(FTileHitResult& TileHitResult) const;
	bool TryGetTilesByRangeFromTile(const ATile* Tile, const FBFSRange& InRange, const ETileRangeQueryType QueryType, TArray<ATile*>& OutTiles) const;
	bool TryGetTilesByRangeFromActor(const AActor* Actor, const FBFSRange& InRange, const ETileRangeQueryType QueryType, TArray<ATile*>& OutTiles) const;

private:
	TMap<EHighlightReason, TArray<TScriptInterface<IHighlightInterface>>> CurrentHighlightedActorsByReason;
};
