// Copyright JETBLU, Inc. All Rights Reserved.

#include "ArrowRenderer.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Character.h"

AArrowRenderer::AArrowRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(SceneRoot);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(RootComponent);

	SplineMesh = CreateDefaultSubobject<USplineMeshComponent>(TEXT("SplineMesh"));
	SplineMesh->SetupAttachment(RootComponent);
	SplineMesh->SetForwardAxis(ESplineMeshAxis::Y);
	SplineMesh->SetVisibility(false);

	ArrowHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowHead"));
	ArrowHead->SetupAttachment(RootComponent);
	ArrowHead->SetVisibility(false);
}

void AArrowRenderer::BeginPlay()
{
	Super::BeginPlay();

	ArrowBodyDynamicMaterialInstance = UMaterialInstanceDynamic::Create(ArrowBodyMaterial, this);
	SplineMesh->SetMaterial(0, ArrowBodyDynamicMaterialInstance);

	ArrowBodyDynamicMaterialInstance->SetScalarParameterValue(FlowSpeedParamName, FlowSpeed);
}

void AArrowRenderer::SetPoints(const AActor* SourceActor, const AActor* TargetActor, const bool bRenderArrowHead) const
{
	if (!SourceActor || !TargetActor)
	{
		return;
	}
	
	const FVector StartLocation = SourceActor->GetActorLocation();
	const FVector EndLocation = TargetActor->GetActorLocation();

	// 두 캐릭터가 다른 경우에만 ArrowBody를 표시합니다.
	if (SourceActor != TargetActor)
	{
		// 두 위치의 방향과 길이를 계산합니다.
		const FVector Direction = EndLocation - StartLocation;
		const FVector NormalizedDirection = Direction.GetSafeNormal();
		const float Distance = Direction.Size();
		ArrowBodyDynamicMaterialInstance->SetScalarParameterValue(TilingParamName, Distance / 200);

		// 시작점과 끝점이 캐릭터와 겹치지 않도록 각각 알맞은 방향으로 보정합니다.
		constexpr float LocationOffset = 50.f;
		const FVector AdjustedStartLocation = StartLocation + NormalizedDirection * LocationOffset;
		const FVector AdjustedEndLocation = EndLocation - NormalizedDirection * LocationOffset * 1.5f;

		// 높이를 거리 비례로 계산한 후, Start와 End의 중간 지점에서 해당 높이만큼 더해 포물선의 중간 지점을 계산합니다.
		const float ArcHeight = FMath::Clamp(Distance * 0.15f, 20.f, 240.f);
		const FVector MidLocation = (AdjustedStartLocation + AdjustedEndLocation) * 0.5f + FVector::UpVector * ArcHeight;

		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(AdjustedStartLocation, ESplineCoordinateSpace::Local, false);
		Spline->AddSplinePoint(MidLocation, ESplineCoordinateSpace::Local, false);
		Spline->AddSplinePoint(AdjustedEndLocation, ESplineCoordinateSpace::Local, true);

		const FVector MeshStart = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);
		const FVector MeshEnd = Spline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::Local);
		const FVector MeshStartTan = Spline->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::Local);
		const FVector MeshEndTan = Spline->GetTangentAtSplinePoint(2, ESplineCoordinateSpace::Local);
		SplineMesh->SetStartAndEnd(MeshStart, MeshStartTan, MeshEnd, MeshEndTan);
		SplineMesh->SetVisibility(true);
	}
	else
	{
		SplineMesh->SetVisibility(false);
	}

	if (bRenderArrowHead)
	{
		FVector ArrowHeadLocation = EndLocation;
		if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
		{
			ArrowHeadLocation.Z += TargetCharacter->GetDefaultHalfHeight() * 2.f;
		}
		ArrowHead->SetWorldLocation(ArrowHeadLocation);
		ArrowHead->SetVisibility(true);
	}
}

void AArrowRenderer::SetActive(const bool bActive) const
{
	SplineMesh->SetVisibility(bActive);
	ArrowHead->SetVisibility(bActive);
}
