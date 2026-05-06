// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerCharacterBase.h"

#include "Components/WidgetComponent.h"
#include "Lethe/Manager/EngineSystem/LetheAssetManager.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	MarkerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidgetComponent"));
	MarkerWidgetComponent->SetupAttachment(RootComponent);
}

FGameplayTag APlayerCharacterBase::GetCharacterTag()
{
	if (!CharacterTag.IsValid())
	{
		const ULetheAssetManager& LetheAssetManager = ULetheAssetManager::Get();
		LetheAssetManager.TryGetCharacterTagById(CharacterId, CharacterTag);
	}
	return CharacterTag;
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
