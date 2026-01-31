#include "Tile.h"

ATile::ATile(const FObjectInitializer& ObjectInitializer)
{
	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(Root);
	
	MainTile = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MainTile"));
	MainTile->SetupAttachment(Root);
}

void ATile::Init(const TArray<UStaticMesh*>& Meshes, const FCubeCoord& InCubeCoord, const int32 RoomID, const bool bIsTop)
{
	TextRender = FindComponentByClass<UTextRenderComponent>();
	SetTileMesh(Meshes);

	CubeCoord = InCubeCoord;
	
	if (bIsTop)
	{
		TextRender->SetText(FText::Format(FText::FromString(TEXT("[{0}, {1}, {2}]\nRoom : {3}")), CubeCoord.Q, CubeCoord.R, CubeCoord.S, RoomID));
	}

	TextRender->SetVisibility(bIsTop);
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

void ATile::HighlightActorByMouse_Implementation()
{
	if (TopTile.IsValid())
	{
		Execute_HighlightActorByMouse(TopTile.Get());
	}
	else
	{
		// 이 타일이 꼭대기 타일인 경우 들어오는 분기입니다.
		MainTile->SetRenderCustomDepth(true);
		MainTile->SetCustomDepthStencilValue(OutlineColorByMouse);
	}
}

void ATile::UnhighlightActorByMouse_Implementation()
{
	if (TopTile.IsValid())
	{
		Execute_UnhighlightActorByMouse(TopTile.Get());
	}
	else
	{
		if (OutlineColorByCard != 0)
		{
			// 기존에 카드에 의해 하이라이팅 되고 있었다면 그 색깔로 되돌립니다.
			MainTile->SetCustomDepthStencilValue(OutlineColorByCard);
		}
		else
		{
			MainTile->SetRenderCustomDepth(false);
		}
	}
}

void ATile::HighlightActorByCard_Implementation(const int32 InOutlineColor)
{
	if (TopTile.IsValid())
	{
		Execute_HighlightActorByCard(TopTile.Get(), InOutlineColor);
	}
	else
	{
		OutlineColorByCard = InOutlineColor;
		MainTile->SetRenderCustomDepth(true);
		MainTile->SetCustomDepthStencilValue(OutlineColorByCard);
	}
}

void ATile::UnhighlightActorByCard_Implementation()
{
	if (TopTile.IsValid())
	{
		Execute_UnhighlightActorByCard(TopTile.Get());
	}
	else
	{
		OutlineColorByCard = 0;
		MainTile->SetRenderCustomDepth(false);
	}
}

FCubeCoord ATile::GetCubeCoord() const
{
	return CubeCoord;
}

void ATile::SetTileMesh(const TArray<UStaticMesh*>& Meshes) const
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	int32 Index = 0;

	for (UStaticMeshComponent* Component : Components)
	{
		Component->SetStaticMesh(Meshes[Index]);
		Index++;
	}
}
