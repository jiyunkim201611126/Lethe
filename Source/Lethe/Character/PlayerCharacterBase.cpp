// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"

APlayerCharacterBase::APlayerCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GetCapsuleComponent()->SetGenerateOverlapEvents(false);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

FGameplayTag APlayerCharacterBase::GetCharacterTag()
{
	if (!CharacterTag.IsValid())
	{
		if (const UDataLoadManagerSubsystem* DataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>())
		{
			CharacterTag = DataLoadManagerSubsystem->GetCharacterTagById(CharacterId);
		}
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
