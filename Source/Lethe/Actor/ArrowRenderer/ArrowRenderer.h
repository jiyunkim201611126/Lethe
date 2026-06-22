// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowRenderer.generated.h"

class APlayerCharacterBase;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

namespace ESplinePointType
{
	enum Type : int;
}

class USplineComponent;
class USplineMeshComponent;

UCLASS(Abstract)
class LETHE_API AArrowRenderer : public AActor
{
	GENERATED_BODY()

public:
	AArrowRenderer();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface
	
	void DrawCardPreviewArrow(const AActor* SourceActor, const TArray<AActor*>& TargetActors);
	void DrawMovePreviewArrow(TMap<APlayerCharacterBase*, TArray<FVector>>& MovePathLocations);
	void DeactivateCardPreviewArrow();
	void DeactivateMovePreviewArrow();

private:
	void SetAllSplinePointsType(const ESplinePointType::Type PointType) const;
	
	void EnsureCardPreviewSplineMeshCount(int32 RequiredCount);
	USplineMeshComponent* CreateCardPreviewSplineMeshComponent();
	void EnsureCardPreviewArrowHeadCount(int32 RequiredCount);
	UStaticMeshComponent* CreateCardPreviewArrowHeadComponent();
	void EnsureMovePreviewSplineMeshCount(int32 RequiredCount);
	USplineMeshComponent* CreateMovePreviewSplineMeshComponent();

protected:
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Mesh")
	TObjectPtr<UStaticMesh> ArrowBody;

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Mesh")
	TObjectPtr<UStaticMesh> ArrowHead;

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	TObjectPtr<UMaterialInterface> ArrowBodyMaterial;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> CardPreviewArrowBodies;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> CardPreviewArrowHeads;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> MovePreviewSplineMeshComponents;

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	FName FlowSpeedParamName = TEXT("FlowSpeed");

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	FName TilingParamName = TEXT("Tiling");

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	float FlowSpeed = 1.f;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> CardPreviewDynamicMaterialInstances;

	UPROPERTY(Transient)
	TMap<TWeakObjectPtr<APlayerCharacterBase>, TObjectPtr<UMaterialInstanceDynamic>> MovePreviewDynamicMaterialInstances;
};
