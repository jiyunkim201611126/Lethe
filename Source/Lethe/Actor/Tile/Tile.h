// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Lethe/Lethe.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Tile.generated.h"

UCLASS()
class LETHE_API ATile : public AActor, public IHighlightInterface
{
	GENERATED_BODY()

public:
	ATile(const FObjectInitializer& ObjectInitializer);
	
	void Init(const TArray<UStaticMesh*>& Meshes, const FCubeCoord& InCubeCoord, const int32 RoomID, const TArray<int32>& UVOffsetType);
	void SetTopTile(ATile* InTile);
	ATile* GetTopTile();

	//~ Begin IHighlightInterface
	virtual void HighlightActorByMouse_Implementation() override;
	virtual void UnhighlightActorByMouse_Implementation() override;
	virtual void HighlightActorByCard_Implementation(const int32 InOutlineColor) override;
	virtual void UnhighlightActorByCard_Implementation() override;
	//~ End of IHighlightInterface
	
	FCubeCoord GetCubeCoord() const;

protected:
	UFUNCTION(BlueprintPure)
	bool IsTopTile() const;

private:
	void SetTileMesh(const TArray<UStaticMesh*>& Meshes, const TArray<int32>& UVOffsetType) const;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UStaticMeshComponent> MainTile;

private:
	// 이 타일의 좌표입니다.
	FCubeCoord CubeCoord;

	// 해당 타일이 TopTile이면 nullptr입니다.
	TWeakObjectPtr<ATile> TopTile;

	int32 OutlineColorByMouse = CUSTOM_DEPTH_RED;
	int32 OutlineColorByCard = 0;
};
