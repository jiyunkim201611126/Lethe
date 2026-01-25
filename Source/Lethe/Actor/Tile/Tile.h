// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/TextRenderComponent.h"
#include "Tile.generated.h"

UCLASS()
class LETHE_API ATile : public AActor
{
	GENERATED_BODY()

public:
	void Init(const TArray<UStaticMesh*>& Meshes, const int32 Q, const int32 R, const int32 S, const int32 RoomID, const bool bIsTop);

private:
	void SetTileMesh(const TArray<UStaticMesh*>& Meshes) const;
	
protected:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UTextRenderComponent> TextRender;
};
