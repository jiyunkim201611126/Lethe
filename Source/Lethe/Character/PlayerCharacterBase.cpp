// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerCharacterBase.h"

#include "Component/PlayerGASManagerComponent.h"
#include "Components/WidgetComponent.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Manager/Tile/RoomManagerSubsystem.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPlayerGASManagerComponent>(TEXT("GASManagerComponent")))
{
	MarkerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComponent"));
	MarkerWidgetComponent->SetupAttachment(RootComponent);
	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));
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

void APlayerCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetPlayerAttributeSet(PlayerAttributeSet);
	Super::InitAbilityActorInfo();
}

void APlayerCharacterBase::OnMoveTileChanged(ATile* OldTile, ATile* NewTile)
{
	if (URoomManagerSubsystem* RoomManagerSubsystem = GetWorld()->GetSubsystem<URoomManagerSubsystem>())
	{
		RoomManagerSubsystem->NotifyCharacterTileChanged(this, OldTile, NewTile);
	}
}
