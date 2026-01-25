#include "Tile.h"

void ATile::Init(const TArray<UStaticMesh*>& Meshes, const int32 Q, const int32 R, const int32 S, const int32 RoomID, const bool bIsTop)
{
	TextRender = FindComponentByClass<UTextRenderComponent>();
	SetTileMesh(Meshes);
	
	if (bIsTop)
	{
		TextRender->SetText(FText::Format(FText::FromString(TEXT("[{0}, {1}, {2}]\nRoom : {3}")), Q, R, S, RoomID));
	}

	TextRender->SetVisibility(bIsTop);
}

void ATile::SetTileMesh(const TArray<UStaticMesh*>& Meshes) const
{
	TArray<UStaticMeshComponent*> Components;
	GetComponents(Components);
	int32 Index = 0;

	for(auto* Component : Components)
	{
		Component->SetStaticMesh(Meshes[Index]);
		Index++;
	}
}
