// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Lethe/Lethe.h"
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

UCLASS()
class LETHE_API ATile : public AActor, public IHighlightInterface
{
	GENERATED_BODY()

public:
	ATile(const FObjectInitializer& ObjectInitializer);
	
	void Init(const TArray<UStaticMesh*>& Meshes, const FCubeCoord& InCubeCoord, const int32 RoomID, const TArray<ETileConnectionState>& UVOffsetType);
	void SetTopTile(ATile* InTile);
	ATile* GetTopTile();

	//~ Begin IHighlightInterface
	virtual void HighlightActorByMouse_Implementation() override;
	virtual void UnhighlightActorByMouse_Implementation() override;
	virtual void HighlightActorByAbility_Implementation(const int32 InOutlineColor) override;
	virtual void UnhighlightActorByAbility_Implementation() override;
	//~ End of IHighlightInterface
	
	FCubeCoord GetCubeCoord() const;

protected:
	UFUNCTION(BlueprintPure)
	bool IsTopTile() const;

private:
	void SetTileMesh(const TArray<UStaticMesh*>& Meshes, const TArray<ETileConnectionState>& UVOffsetType) const;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MainTile;

private:
	FCubeCoord CubeCoord;

	/** TopTile 변수의 값이 nullptr이면, 해당 객체가 가장 윗단에 위치한 TopTile입니다. */
	TWeakObjectPtr<ATile> TopTile;

	int32 OutlineColorByMouse = CUSTOM_DEPTH_RED;
	int32 OutlineColorByCard = 0;
};
