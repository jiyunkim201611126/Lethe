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
	FGameplayTag Damage_Normal;

	TArray<FGameplayTag> DamageTypeTags;
	//~ End of Damage Types
	
	//~ Begin Abilities
	FGameplayTag Abilities_Test;
	//~ End of Abilities
	
	//~ Begin Ability Types
	FGameplayTag Abilities_Types_Active;
	FGameplayTag Abilities_Types_Passive;
	//~ End Ability Types

	//~ Begin CharacterState
	FGameplayTag CharacterState_Dead;
	FGameplayTag CharacterState_Knockback;
	//~ End of CharacterState

private:
	static FLetheGameplayTags GameplayTags;
};
