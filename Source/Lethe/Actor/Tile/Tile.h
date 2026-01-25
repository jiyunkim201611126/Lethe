// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Lethe/Data/Stage/CubeCoord.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Tile.generated.h"

UCLASS()
class LETHE_API ATile : public AActor, public IHighlightInterface
{
	GENERATED_BODY()

public:
	void Init(const TArray<UStaticMesh*>& Meshes, const FCubeCoord& InCubeCoord, const int32 RoomID, const bool bIsTop);
	const FCubeCoord& GetCubeCoord() const;
	
	//~ Begin IHighlightInterface
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
	//~ End of IHighlightInterface

private:
	void SetTileMesh(const TArray<UStaticMesh*>& Meshes) const;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;

private:
	FCubeCoord CubeCoord;
};
