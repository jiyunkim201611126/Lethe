// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerCharacterBase.h"

#include "Components/WidgetComponent.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MarkerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComponent"));
	MarkerWidgetComponent->SetupAttachment(RootComponent);
}

void APlayerCharacterBase::SetPersonalColor(const FLinearColor& InColor)
{
	PersonalColor = InColor;
}

const FLinearColor& APlayerCharacterBase::GetPersonalColor() const
{
	return PersonalColor;
}

void APlayerCharacterBase::SetPlayerOrderIndex(const int32 Index)
{
	PlayerOrderIndex = Index;
}

int32 APlayerCharacterBase::GetPlayerOrderIndex() const
{
	return PlayerOrderIndex;
}

void APlayerCharacterBase::OnMoveTileChanged(ATile* OldTile, ATile* NewTile)
{
	if (URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>())
	{
		RoomManagerSubsystem->NotifyCharacterTileChanged(this, OldTile, NewTile);
	}
}
