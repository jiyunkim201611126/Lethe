// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Tile.generated.h"

UENUM()
enum class ETileConnectionState
{
	Block,
	Connected,
	VerticalConnected,
};

UENUM(BlueprintType)
enum class ETileVisionState : uint8
{
	None,
	
	/** 아직 볼 수 없는 타일로, 렌더링 자체가 꺼진 상태입니다. */
	Hidden,
	/** 현재 볼 수 있는 타일로, 방문한 Room 내 모든 타일과 연결된 Room의 입구 타일까지 해당합니다. */
	Visible,
	/** 타일의 형태는 보이나 그 위 액터는 보이지 않는 상태입니다. */
	Recognizable
};

UCLASS(meta = (PrioritizeCategories = "Vision"))
class LETHE_API ATile : public AActor, public IHighlightInterface
{
	GENERATED_BODY()

public:
	ATile(const FObjectInitializer& ObjectInitializer);

	/** 초기화 단계에 호출되는 함수기 때문에, TopTile을 아직 할당받지 않아 bIsTopTile을 직접 받아 사용합니다. */
	void Init(const TArray<UStaticMesh*>& Meshes, const FCubeCoord& InCubeCoord, const int32 InRoomId, const TArray<ETileConnectionState>& UVOffsetType, const bool bIsTopTile);
	void SetTopTile(ATile* InTile);
	ATile* GetTopTile();

	//~ Begin IHighlightInterface
	virtual void Highlight_Implementation(const EHighlightReason Reason) override;
	virtual void Unhighlight_Implementation(const EHighlightReason Reason) override;
	//~ End of IHighlightInterface

	void SetTileVisionState(const ETileVisionState VisionState);

	UFUNCTION(BlueprintPure)
	ETileVisionState GetTileVisionState() const;
	
	FCubeCoord GetCubeCoord() const;
	int32 GetRoomId() const;

	void AddOccupiedCount();
	void SubtractOccupiedCount();
	int32 GetOccupiedCount() const;

	UFUNCTION(BlueprintPure)
	bool IsTopTile() const;

private:
	void SetTileMesh(const TArray<UStaticMesh*>& Meshes, const TArray<ETileConnectionState>& UVOffsetType) const;
	void SetTileTraceIgnore(const bool bIgnore) const;
	
	void RefreshHighlight() const;
	int32 ResolveHighlightStencilValue() const;

protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MainTile;

	/** false면 타일 시야 로직을 적용하지 않습니다. */
	UPROPERTY(EditDefaultsOnly, Category = "Vision")
	bool bUseTileVisionLogic = true;

private:
	FCubeCoord CubeCoord;
	int32 RoomId = INDEX_NONE;

	/** TopTile 변수의 값이 nullptr이면, 해당 객체가 가장 윗단에 위치한 TopTile입니다. */
	TWeakObjectPtr<ATile> TopTile;

	/** PlayerMovePhase에 사용되는 값으로, 플레이어 캐릭터가 경로 예약 시 해당 타일을 지나치는 경우 1씩 늘어납니다. */
	UPROPERTY(VisibleInstanceOnly)
	int32 OccupiedCount = 0;

	ETileVisionState TileVisionState = ETileVisionState::None;

	/** 여러 객체가  */
	TMap<EHighlightReason, int32> HighlightReasonCounts;

public:
	void Debug_Noise() const;
};
