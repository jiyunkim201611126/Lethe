// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FLetheGameplayTags
{
	static const FLetheGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();

	//~ Begin Attributes
	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_MaxHealth;
	//~ End of Attributes
	
	//~ Begin Damage Types
	FGameplayTag Damage_Physical;

	TArray<FGameplayTag> DamageTypeTags;
	//~ End of Damage Types
	
	//~ Begin Abilities
	FGameplayTag Card_Test;
	//~ End of Abilities
	
	//~ Begin Ability Types
	FGameplayTag Card_Types_Physical;
	FGameplayTag Card_Types_Magic;
	FGameplayTag Card_Types_Util;
	//~ End Ability Types

	//~ Begin CharacterState
	FGameplayTag CharacterState_Dead;
	FGameplayTag CharacterState_Knockback;
	//~ End of CharacterState

private:
	static FLetheGameplayTags GameplayTags;
};
