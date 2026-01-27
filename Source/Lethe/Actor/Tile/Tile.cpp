#include "Tile.h"

#include "Lethe/Lethe.h"

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

const FCubeCoord& ATile::GetCubeCoord() const
{
	return CubeCoord;
}

void ATile::HighlightActor()
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	
	for (UStaticMeshComponent* Component : Components)
	{
		Component->SetRenderCustomDepth(true);
	}
}

void ATile::UnHighlightActor()
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	
	for (UStaticMeshComponent* Component : Components)
	{
		Component->SetRenderCustomDepth(false);
	}
}

void ATile::SetActorOnTile(AActor* InActor)
{
	ActorOnTile = InActor;
}

void ATile::SetTileMesh(const TArray<UStaticMesh*>& Meshes) const
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	int32 Index = 0;

	for (UStaticMeshComponent* Component : Components)
	{
		Component->SetStaticMesh(Meshes[Index]);
		Component->CustomDepthStencilValue = CUSTOM_DEPTH_RED;
		Index++;
	}
}
