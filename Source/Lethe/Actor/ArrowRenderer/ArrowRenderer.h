// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowRenderer.generated.h"

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

	void InitializeMovePreviewSplineMeshes(int32 InitialMeshCount);
	
	void DrawSkillPreviewArrow(const AActor* SourceActor, const AActor* TargetActor, const bool bRenderArrowHead = true) const;
	void DrawMovePreviewArrow(const TArray<TArray<FVector>>& MovePathLocations);
	void DeactivateArrow();

private:
	void SetAllSplinePointsType(const ESplinePointType::Type PointType) const;
	
	void EnsureMovePreviewSplineMeshCount(int32 RequiredCount);
	USplineMeshComponent* CreateMovePreviewSplineMeshComponent();

protected:
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineComponent> Spline;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineMeshComponent> SkillPreviewSplineMeshComponent;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USplineMeshComponent>> MovePreviewSplineMeshComponents;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> ArrowHead;

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	TObjectPtr<UMaterialInterface> ArrowBodyMaterial;

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	FName FlowSpeedParamName = TEXT("FlowSpeed");

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	FName TilingParamName = TEXT("Tiling");

	UPROPERTY(EditDefaultsOnly, Category = "Arrow | Material")
	float FlowSpeed = 1.f;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ArrowBodyDynamicMaterialInstance;
};
