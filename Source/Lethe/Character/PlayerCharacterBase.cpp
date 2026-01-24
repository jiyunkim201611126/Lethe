// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerCharacterBase.h"

#include "Lethe/Manager/DataLoadManagerSubsystem.h"

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
