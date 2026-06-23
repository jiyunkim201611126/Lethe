// Copyright JETBLU, Inc. All Rights Reserved.

#include "ArrowRenderer.h"

#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Lethe/Character/PlayerCharacterBase.h"
#include "Materials/MaterialInstanceDynamic.h"

AArrowRenderer::AArrowRenderer()
{
	PrimaryActorTick.bCanEverTick = false;

	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(SceneRoot);

	Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Spline"));
	Spline->SetupAttachment(RootComponent);
}

void AArrowRenderer::BeginPlay()
{
	Super::BeginPlay();
}

void AArrowRenderer::DrawCardPreviewArrow(const AActor* SourceActor, const TArray<AActor*>& TargetActors)
{
	DeactivateCardPreviewArrow();
	if (!SourceActor || TargetActors.IsEmpty())
	{
		return;
	}

	// 시작점을 계산하고, Arrow를 필요한 만큼 생성합니다.
	const FVector StartLocation = SourceActor->GetActorLocation();
	int32 RequiredArrowBodyCount = 0;
	for (const AActor* TargetActor : TargetActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		if (SourceActor != TargetActor)
		{
			++RequiredArrowBodyCount;
		}
	}
	EnsureCardPreviewSplineMeshCount(RequiredArrowBodyCount);
	EnsureCardPreviewArrowHeadCount(RequiredArrowBodyCount);

	int32 ArrowBodyIndex = 0;
	int32 ArrowHeadIndex = 0;
	for (const AActor* TargetActor : TargetActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		const FVector EndLocation = TargetActor->GetActorLocation();

		// 두 캐릭터가 다른 경우에만 ArrowBody를 표시합니다.
		if (SourceActor == TargetActor)
		{
			continue;
		}

		if (!CardPreviewArrowBodies.IsValidIndex(ArrowBodyIndex))
		{
			return;
		}

		// 두 위치의 방향과 길이를 계산합니다.
		const FVector Direction = EndLocation - StartLocation;
		const FVector NormalizedDirection = Direction.GetSafeNormal();
		const float Distance = Direction.Size();
		if (CardPreviewDynamicMaterialInstances.IsValidIndex(ArrowBodyIndex) && CardPreviewDynamicMaterialInstances[ArrowBodyIndex])
		{
			CardPreviewDynamicMaterialInstances[ArrowBodyIndex]->SetScalarParameterValue(TilingParamName, Distance / 200.f);
		}

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
		SetAllSplinePointsType(ESplinePointType::Curve);

		const FVector StartPos = Spline->GetLocationAtSplinePoint(0, ESplineCoordinateSpace::Local);
		const FVector StartTangent = Spline->GetTangentAtSplinePoint(0, ESplineCoordinateSpace::Local);
		const FVector EndPos = Spline->GetLocationAtSplinePoint(2, ESplineCoordinateSpace::Local);
		const FVector EndTangent = Spline->GetTangentAtSplinePoint(2, ESplineCoordinateSpace::Local);
		CardPreviewArrowBodies[ArrowBodyIndex]->SetStartAndEnd(StartPos, StartTangent, EndPos, EndTangent);
		CardPreviewArrowBodies[ArrowBodyIndex]->SetVisibility(true);
		++ArrowBodyIndex;

		if (CardPreviewArrowHeads.IsValidIndex(ArrowHeadIndex))
		{
			FVector ArrowHeadLocation = EndLocation;
			if (const ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor))
			{
				ArrowHeadLocation.Z += TargetCharacter->GetDefaultHalfHeight() * 2.f;
			}
			CardPreviewArrowHeads[ArrowHeadIndex]->SetWorldLocation(ArrowHeadLocation);
			CardPreviewArrowHeads[ArrowHeadIndex]->SetVisibility(true);
			++ArrowHeadIndex;
		}
	}
}

void AArrowRenderer::DrawMovePreviewArrow(TMap<APlayerCharacterBase*, TArray<FVector>>& MovePathLocations)
{
	DeactivateMovePreviewArrow();

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
		APlayerCharacterBase* PlayerCharacter = MovePathLocation.Key;
		if (!PlayerCharacter)
		{
			continue;
		}
		
		// PlayerCharacter의 PersonalColor를 사용하는 MaterialInstance를 가져오거나 생성합니다.
		auto& MovePreviewMaterialInstance = MovePreviewDynamicMaterialInstances.FindOrAdd(PlayerCharacter);
		if (!MovePreviewMaterialInstance)
		{
			MovePreviewMaterialInstance = UMaterialInstanceDynamic::Create(ArrowBodyMaterial, this);
			if (MovePreviewMaterialInstance)
			{
				MovePreviewMaterialInstance->SetVectorParameterValue(TEXT("Color"), PlayerCharacter->GetPersonalColor());
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

void AArrowRenderer::DeactivateCardPreviewArrow()
{
	Spline->ClearSplinePoints(false);

	for (USplineMeshComponent* SplineMeshComponent : CardPreviewArrowBodies)
	{
		if (SplineMeshComponent)
		{
			SplineMeshComponent->SetVisibility(false);
		}
	}

	for (UStaticMeshComponent* ArrowHeadComponent : CardPreviewArrowHeads)
	{
		if (ArrowHeadComponent)
		{
			ArrowHeadComponent->SetVisibility(false);
		}
	}
}

void AArrowRenderer::DeactivateMovePreviewArrow()
{
	for (USplineMeshComponent* SplineMeshComponent : MovePreviewSplineMeshComponents)
	{
		SplineMeshComponent->SetVisibility(false);
	}
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

void AArrowRenderer::EnsureCardPreviewSplineMeshCount(const int32 RequiredCount)
{
	while (CardPreviewArrowBodies.Num() < RequiredCount)
	{
		USplineMeshComponent* NewSplineMeshComponent = CreateCardPreviewSplineMeshComponent();
		if (!NewSplineMeshComponent)
		{
			return;
		}

		CardPreviewArrowBodies.Add(NewSplineMeshComponent);
	}
}

USplineMeshComponent* AArrowRenderer::CreateCardPreviewSplineMeshComponent()
{
	if (!ArrowBody)
	{
		return nullptr;
	}

	const int32 ComponentIndex = CardPreviewArrowBodies.Num();
	const FName ComponentName = *FString::Printf(TEXT("CardPreviewSplineMesh_%d"), ComponentIndex);
	USplineMeshComponent* NewSplineMeshComponent = NewObject<USplineMeshComponent>(this, ComponentName);
	if (!NewSplineMeshComponent)
	{
		return nullptr;
	}

	NewSplineMeshComponent->SetupAttachment(RootComponent);
	NewSplineMeshComponent->SetForwardAxis(ESplineMeshAxis::Y);
	NewSplineMeshComponent->SetVisibility(false);
	NewSplineMeshComponent->SetStaticMesh(ArrowBody);
	NewSplineMeshComponent->SetMobility(EComponentMobility::Movable);

	UMaterialInstanceDynamic* DynamicMaterialInstance = UMaterialInstanceDynamic::Create(ArrowBodyMaterial, this);
	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(FlowSpeedParamName, FlowSpeed);
		NewSplineMeshComponent->SetMaterial(0, DynamicMaterialInstance);
	}
	CardPreviewDynamicMaterialInstances.Add(DynamicMaterialInstance);

	AddInstanceComponent(NewSplineMeshComponent);
	NewSplineMeshComponent->RegisterComponent();

	return NewSplineMeshComponent;
}

void AArrowRenderer::EnsureCardPreviewArrowHeadCount(const int32 RequiredCount)
{
	while (CardPreviewArrowHeads.Num() < RequiredCount)
	{
		UStaticMeshComponent* NewArrowHeadComponent = CreateCardPreviewArrowHeadComponent();
		if (!NewArrowHeadComponent)
		{
			return;
		}

		CardPreviewArrowHeads.Add(NewArrowHeadComponent);
	}
}

UStaticMeshComponent* AArrowRenderer::CreateCardPreviewArrowHeadComponent()
{
	if (!ArrowHead)
	{
		return nullptr;
	}

	const FName ComponentName = *FString::Printf(TEXT("CardPreviewArrowHead_%d"), CardPreviewArrowHeads.Num());
	UStaticMeshComponent* NewArrowHeadComponent = NewObject<UStaticMeshComponent>(this, ComponentName);
	if (!NewArrowHeadComponent)
	{
		return nullptr;
	}

	NewArrowHeadComponent->SetupAttachment(RootComponent);
	NewArrowHeadComponent->SetVisibility(false);
	NewArrowHeadComponent->SetStaticMesh(ArrowHead);
	NewArrowHeadComponent->SetMobility(EComponentMobility::Movable);

	AddInstanceComponent(NewArrowHeadComponent);
	NewArrowHeadComponent->RegisterComponent();

	return NewArrowHeadComponent;
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

		MovePreviewSplineMeshComponents.Add(NewSplineMeshComponent);
	}
}

USplineMeshComponent* AArrowRenderer::CreateMovePreviewSplineMeshComponent()
{
	if (!ArrowBody)
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
	NewSplineMeshComponent->SetStaticMesh(ArrowBody);
	NewSplineMeshComponent->SetMobility(EComponentMobility::Movable);

	AddInstanceComponent(NewSplineMeshComponent);
	NewSplineMeshComponent->RegisterComponent();

	return NewSplineMeshComponent;
}
