// Copyright JETBLU, Inc. All Rights Reserved.

#include "ArrowRenderer.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "GameFramework/Character.h"
#include "Lethe/Character/PlayerCharacterBase.h"

AArrowRenderer::AArrowRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(SceneRoot);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(RootComponent);

	SkillPreviewSplineMeshComponent = CreateDefaultSubobject<USplineMeshComponent>(TEXT("SkillPreviewSplineMesh"));
	SkillPreviewSplineMeshComponent->SetupAttachment(RootComponent);
	SkillPreviewSplineMeshComponent->SetForwardAxis(ESplineMeshAxis::Y);
	SkillPreviewSplineMeshComponent->SetVisibility(false);
	SkillPreviewSplineMeshComponent->SetMobility(EComponentMobility::Movable);

	ArrowHead = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowHead"));
	ArrowHead->SetupAttachment(RootComponent);
	ArrowHead->SetVisibility(false);
}

void AArrowRenderer::BeginPlay()
{
	Super::BeginPlay();

	ArrowBodyDynamicMaterialInstance = UMaterialInstanceDynamic::Create(ArrowBodyMaterial, this);
	SkillPreviewSplineMeshComponent->SetMaterial(0, ArrowBodyDynamicMaterialInstance);

	ArrowBodyDynamicMaterialInstance->SetScalarParameterValue(FlowSpeedParamName, FlowSpeed);
}

void AArrowRenderer::InitializeMovePreviewSplineMeshes(const int32 InitialMeshCount)
{
	EnsureMovePreviewSplineMeshCount(InitialMeshCount);
}

void AArrowRenderer::DrawSkillPreviewArrow(const AActor* SourceActor, const AActor* TargetActor, const bool bRenderArrowHead) const
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
		Spline->AddSplinePoint(AdjustedEndLocation, ESplineCoordinateSpace::Local, false);

		const FVector StartPos = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);
		const FVector StartTangent = Spline->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::Local);
		const FVector EndPos = Spline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::Local);
		const FVector EndTangent = Spline->GetTangentAtSplinePoint(2, ESplineCoordinateSpace::Local);
		SkillPreviewSplineMeshComponent->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		SkillPreviewSplineMeshComponent->SetVisibility(true);

		SetAllSplinePointsType(ESplinePointType::Curve);
	}
	else
	{
		SkillPreviewSplineMeshComponent->SetVisibility(false);
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

void AArrowRenderer::DrawMovePreviewArrow(TMap<APlayerCharacterBase*, TArray<FVector>>& MovePathLocations)
{
	DeactivateArrow();

	// SplineMeshComponent를 필요한 만큼 생성합니다.
	int32 RequiredMeshCount = 0;
	for (const auto& MovePathLocation : MovePathLocations)
	{
		RequiredMeshCount += FMath::Max(MovePathLocation.Value.Num() - 1, 0);
	}
	EnsureMovePreviewSplineMeshCount(RequiredMeshCount);

	// 타일 사이마다 직선으로 표시되는 SplineMeshComponent를 하나씩 배치합니다.
	int32 MeshIndex = 0;
	for (const auto& MovePathLocation : MovePathLocations)
	{
		// PlayerCharacter의 PersonalColor를 사용하는 MaterialInstance를 가져오거나 생성합니다.
		UMaterialInstanceDynamic* MovePreviewMaterialInstance = MovePreviewDynamicMaterialInstances.FindRef(MovePathLocation.Key);
		if (!MovePreviewMaterialInstance)
		{
			MovePreviewMaterialInstance = UMaterialInstanceDynamic::Create(ArrowBodyMaterial, this);
			if (MovePreviewMaterialInstance)
			{
				MovePreviewMaterialInstance->SetVectorParameterValue(TEXT("Color"), MovePathLocation.Key->GetPersonalColor());
			}
		}
		
		for (int32 TileIndex = 0; TileIndex < MovePathLocation.Value.Num() - 1; ++TileIndex)
		{
			if (!MovePreviewSplineMeshComponents.IsValidIndex(MeshIndex))
			{
				return;
			}

			const FVector StartPos = MovePathLocation.Value[TileIndex];
			const FVector EndPos = MovePathLocation.Value[TileIndex + 1];
			const FVector SegmentTangent = EndPos - StartPos;
			MovePreviewSplineMeshComponents[MeshIndex]->SetStartAndEnd(StartPos, SegmentTangent, EndPos, SegmentTangent);
			MovePreviewSplineMeshComponents[MeshIndex]->SetVisibility(true);
			MovePreviewSplineMeshComponents[MeshIndex]->SetMaterial(0, MovePreviewMaterialInstance);
			++MeshIndex;
		}
	}
}

void AArrowRenderer::DeactivateArrow()
{
	Spline->ClearSplinePoints(false);

	if (SkillPreviewSplineMeshComponent)
	{
		SkillPreviewSplineMeshComponent->SetVisibility(false);
	}

	for (USplineMeshComponent* SplineMeshComponent : MovePreviewSplineMeshComponents)
	{
		SplineMeshComponent->SetVisibility(false);
	}

	ArrowHead->SetVisibility(false);
}

void AArrowRenderer::SetAllSplinePointsType(const ESplinePointType::Type PointType) const
{
	const int32 NumPoints = Spline->GetNumberOfSplinePoints();

	for (int32 Index = 0; Index < NumPoints; ++Index)
	{
		Spline->SetSplinePointType(Index, PointType);
	}

	Spline->UpdateSpline();
}

void AArrowRenderer::EnsureMovePreviewSplineMeshCount(const int32 RequiredCount)
{
	// 꺾인 직선을 표현하기 위해선 (타일 개수 - 1)만큼의 MeshComponent가 필요합니다.
	// 아주 정확하게는, (타일 개수 - 현재 이동 예약한 캐릭터 개수)지만 그냥 널널하게 생성합니다.
	while (MovePreviewSplineMeshComponents.Num() < RequiredCount)
	{
		USplineMeshComponent* NewSplineMeshComponent = CreateMovePreviewSplineMeshComponent();
		if (!NewSplineMeshComponent)
		{
			return;
		}

		MovePreviewSplineMeshComponents.Emplace(NewSplineMeshComponent);
	}
}

USplineMeshComponent* AArrowRenderer::CreateMovePreviewSplineMeshComponent()
{
	if (!SkillPreviewSplineMeshComponent)
	{
		return nullptr;
	}

	const FName ComponentName = *FString::Printf(TEXT("MovePreviewSplineMesh_%d"), MovePreviewSplineMeshComponents.Num());
	USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(this, ComponentName);
	if (!NewSplineMeshComponent)
	{
		return nullptr;
	}

	NewSplineMeshComponent->SetupAttachment(RootComponent);
	NewSplineMeshComponent->SetForwardAxis(ESplineMeshAxis::Y);
	NewSplineMeshComponent->SetVisibility(false);
	NewSplineMeshComponent->SetStaticMesh(SkillPreviewSplineMeshComponent->GetStaticMesh());
	NewSplineMeshComponent->SetMobility(EComponentMobility::Movable);

	AddInstanceComponent(NewSplineMeshComponent);
	NewSplineMeshComponent->RegisterComponent();

	return NewSplineMeshComponent;
}
