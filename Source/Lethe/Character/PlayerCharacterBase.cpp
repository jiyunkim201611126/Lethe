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
