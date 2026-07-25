// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "Tile.h"

#include "Lethe/Lethe.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ATile::ATile(const FObjectInitializer& ObjectInitializer)
{
	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(Root);
	
	MainTile = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainTile"));
	MainTile->SetupAttachment(Root);
}

void ATile::Init(const TArray<UStaticMesh*>& Meshes, const FCubeCoord& InCubeCoord, const int32 InRoomId, const TArray<ETileConnectionState>& UVOffsetType, const bool bIsTopTile)
{
	TextRender = FindComponentByClass<UTextRenderComponent>();
	SetTileMesh(Meshes, UVOffsetType);

	CubeCoord = InCubeCoord;
	RoomId = InRoomId;

	TextRender->SetVisibility(bIsTopTile);
	if (bIsTopTile)
	{
		TextRender->SetText(FText::Format(FText::FromString(TEXT("[{0}, {1}, {2}]\nRoom : {3}")), CubeCoord.Q, CubeCoord.R, CubeCoord.S, InRoomId));
	}

	if (bUseTileVisionLogic)
	{
		SetTileVisionState(ETileVisionState::Hidden);
	}
}

void ATile::SetTopTile(ATile* InTile)
{
	TopTile = InTile;
}

ATile* ATile::GetTopTile()
{
	if (TopTile.IsValid())
	{
		return TopTile.Get();
	}

	return this;
}

void ATile::SetTileVisionState(const ETileVisionState VisionState)
{
	if (TileVisionState == VisionState)
	{
		return;
	}
	
	if (bUseTileVisionLogic)
	{
		TileVisionState = VisionState;
		switch (TileVisionState)
		{
		case ETileVisionState::Hidden:
			SetActorHiddenInGame(true);
			TextRender->SetVisibility(false);
			SetTileTraceIgnore(true);
			break;
		case ETileVisionState::Visible:
			SetActorHiddenInGame(false);
			TextRender->SetVisibility(IsTopTile());
			SetTileTraceIgnore(false);
			break;
		case ETileVisionState::Recognizable:
			SetActorHiddenInGame(false);
			TextRender->SetVisibility(false);
			SetTileTraceIgnore(false);
			break;
		default:
			break;
		}
	}
}

ETileVisionState ATile::GetTileVisionState() const
{
	return TileVisionState;
}

FCubeCoord ATile::GetCubeCoord() const
{
	return CubeCoord;
}

void ATile::AddOccupiedCount()
{
	++OccupiedCount;
}

void ATile::SubtractOccupiedCount()
{
	--OccupiedCount;
}

int32 ATile::GetOccupiedCount() const
{
	return OccupiedCount;
}

int32 ATile::GetRoomId() const
{
	return RoomId;
}

bool ATile::IsTopTile() const
{
	return !TopTile.IsValid();
}

void ATile::SetTileMesh(const TArray<UStaticMesh*>& Meshes, const TArray<ETileConnectionState>& UVOffsetType) const
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents<UStaticMeshComponent>(Components);
	for (UStaticMeshComponent* Component : Components)
	{
		if (!Component || Component->ComponentTags.IsEmpty())
		{
			continue;
		}
		
		const FString TagString = Component->ComponentTags[0].ToString();
		if (!TagString.IsNumeric())
		{
			continue;
		}
		
		const int32 Direction = FCString::Atoi(*TagString);
		if (0 <= Direction && Direction < 6)
		{
			// Meshes는 MainTile의 Mesh를 0번째 인덱스로 갖고 있으므로 1을 더한 인덱스로 가져옵니다.
			Component->SetStaticMesh(Meshes[Direction + 1]);

			if (IsTopTile())
			{
				// 주변부 타일과의 연결 여부를 머티리얼이 가져갈 수 있도록 내부적으로 값을 할당합니다.
				const float UVOffset = static_cast<float>(UVOffsetType[Direction]);
				MainTile->SetCustomPrimitiveDataFloat(Direction, UVOffset);
				Component->SetCustomPrimitiveDataFloat(0, UVOffset);
			}
		}
	}
}

void ATile::SetTileTraceIgnore(const bool bIgnore) const
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents<UStaticMeshComponent>(Components);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component)
		{
			Component->SetCollisionResponseToChannel(ECC_Tile, bIgnore ? ECR_Ignore : ECR_Block);
		}
	}
}

void ATile::Highlight_Implementation(const EHighlightReason Reason)
{
	if (IsTopTile())
	{
		int32& Count = HighlightReasonCounts.FindOrAdd(Reason);
		++Count;
		RefreshHighlight();
	}
	else
	{
		Execute_Highlight(GetTopTile(), Reason);
	}
}

void ATile::Unhighlight_Implementation(const EHighlightReason Reason)
{
	if (IsTopTile())
	{
		int32& Count = HighlightReasonCounts.FindOrAdd(Reason);
		--Count;
		if (Count <= 0)
		{
			HighlightReasonCounts.Remove(Reason);
		}
		RefreshHighlight();
	}
	else
	{
		Execute_Unhighlight(GetTopTile(), Reason);
	}
}

void ATile::RefreshHighlight() const
{
	const int32 StencilValue = ResolveHighlightStencilValue();
	if (StencilValue == INDEX_NONE)
	{
		MainTile->SetRenderCustomDepth(false);
		return;
	}

	MainTile->SetRenderCustomDepth(true);
	MainTile->SetCustomDepthStencilValue(StencilValue);
}

int32 ATile::ResolveHighlightStencilValue() const
{
	EHighlightReason ActiveHighlightFlags = EHighlightReason::None;
	for (const auto& Pair : HighlightReasonCounts)
	{
		if (Pair.Value > 0)
		{
			ActiveHighlightFlags |= Pair.Key;
		}
	}
	
	if (EnumHasAnyFlags(ActiveHighlightFlags, EHighlightReason::TargetCandidate))
	{
		return CUSTOM_DEPTH_RED;
	}

	if (EnumHasAnyFlags(ActiveHighlightFlags, EHighlightReason::Source))
	{
		return CUSTOM_DEPTH_BLACK;
	}

	if (EnumHasAnyFlags(ActiveHighlightFlags, EHighlightReason::SelectCandidate))
	{
		if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
		{
			if (TileManagerSubsystem->GetActorOnTile(this))
			{
				return CUSTOM_DEPTH_GREEN;
			}
		}
		return CUSTOM_DEPTH_BLUE;
	}

	if (EnumHasAnyFlags(ActiveHighlightFlags, EHighlightReason::TargetedByAI))
	{
		return CUSTOM_DEPTH_PURPLE;
	}

	return INDEX_NONE;
}

void ATile::Debug_Noise() const
{
#if WITH_EDITOR
	FVector DebugBoxLocation = GetActorLocation();
	DebugBoxLocation.Z += 30.f;
	DrawDebugBox(GetWorld(), DebugBoxLocation, FVector(10.f), FColor::Red, false, 3.f);
#endif
}
