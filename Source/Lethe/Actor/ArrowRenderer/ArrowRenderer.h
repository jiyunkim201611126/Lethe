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

	void SetPoints(const FVector& StartLocation, const FVector& EndLocation) const;
	void SetActive(bool bActive) const;

protected:
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineComponent> Spline;
	
	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<USplineMeshComponent> SplineMesh;

	UPROPERTY(VisibleDefaultsOnly)
	TObjectPtr<UStaticMeshComponent> ArrowHead;
};
