// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FLetheGameplayTags
{
	static const FLetheGameplayTags& Get() { return GameplayTags; }
	static void InitializeNativeGameplayTags();
	
	//~ Begin Characters
	FGameplayTag Character_Test0;
	FGameplayTag Character_Test1;
	//~ End of Characters

	//~ Begin Attributes
	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_MaxHealth;
	//~ End of Attributes
	
	//~ Begin Damage Types
	FGameplayTag Damage_Physical;

	TArray<FGameplayTag> DamageTypeTags;
	//~ End of Damage Types
	
	//~ Begin Card Ability
	FGameplayTag Card_Ability_PhysicalTest;
	FGameplayTag Card_Ability_MagicTest;
	FGameplayTag Card_Ability_UtilTest;
	//~ End of Card Ability
	
	//~ Begin Card Types
	FGameplayTag Card_Types_Physical;
	FGameplayTag Card_Types_Magic;
	FGameplayTag Card_Types_Util;
	//~ End Card Types

	//~ Begin CharacterState
	FGameplayTag CharacterState_Dead;
	FGameplayTag CharacterState_Knockback;
	//~ End of CharacterState

private:
	static FLetheGameplayTags GameplayTags;
};
