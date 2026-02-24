// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowRenderer.generated.h"

class USplineComponent;
class USplineMeshComponent;

UCLASS()
class LETHE_API AArrowRenderer : public AActor
{
	GENERATED_BODY()

public:
	AArrowRenderer();

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End of AActor Interface

	void SetPoints(const AActor* SourceActor, const AActor* TargetActor) const;
	void SetActive(bool bActive) const;

protected:
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineComponent> Spline;
	
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineMeshComponent> SplineMesh;

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
