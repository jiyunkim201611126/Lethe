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
	FGameplayTag Character_Test2;
	FGameplayTag Character_Test3;
	//~ End of Characters

	//~ Begin Attributes
	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_MaxHealth;
	FGameplayTag Attributes_Vital_Mana;
	FGameplayTag Attributes_Vital_MaxMana;
	FGameplayTag Attributes_Vital_Cost;
	FGameplayTag Attributes_Vital_MaxCost;
	//~ End of Attributes
	
	//~ Begin Damage Types
	FGameplayTag Damage;
	FGameplayTag Damage_Physical;

	TArray<FGameplayTag> DamageTypeTags;
	//~ End of Damage Types

	//~ Begin Ability
	FGameplayTag Ability_Move;
	//~ End of Ability
	
	//~ Begin Card Ability
	FGameplayTag Card_Ability_PhysicalTest;
	FGameplayTag Card_Ability_MagicTest;
	FGameplayTag Card_Ability_UtilTest;
	FGameplayTag Card_Ability_JustTest;
	//~ End of Card Ability
	
	//~ Begin Card Types
	FGameplayTag Card_Types_Physical;
	FGameplayTag Card_Types_Magic;
	FGameplayTag Card_Types_Util;
	//~ End Card Types

	//~ Begin PhaseState
	FGameplayTag State_Phase_Draw;
	FGameplayTag State_Phase_PlayerTurn;
	//~ End of PhaseState

	//~ Begin CharacterState
	FGameplayTag State_Character_Dead;
	FGameplayTag State_Character_MoveConsumed;
	//~ End of CharacterState

	//~ Begin Montage Event
	FGameplayTag MontageEvent_ApplyEffect;
	FGameplayTag MontageEvent_EndUseCard;
	//~ End of Montage Event

private:
	static FLetheGameplayTags GameplayTags;
};
