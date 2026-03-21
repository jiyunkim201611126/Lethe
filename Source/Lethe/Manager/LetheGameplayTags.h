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
	FGameplayTag Attributes_Vital_ManaRecovery;
	FGameplayTag Attributes_Vital_CostRecovery;
	FGameplayTag Attributes_Meta_IncomingDamage;
	//~ End of Attributes
	
	//~ Begin Damage Types
	FGameplayTag Damage;
	FGameplayTag Damage_Physical_Thrust;

	TArray<FGameplayTag> DamageTypeTags;
	//~ End of Damage Types

	//~ Begin Ability
	FGameplayTag Ability_Move;
	//~ End of Ability
	
	//~ Begin Card Types
	FGameplayTag Card_Types_Physical;
	FGameplayTag Card_Types_Magic;
	FGameplayTag Card_Types_Util;
	//~ End Card Types

	//~ Begin CharacterState
	FGameplayTag State_Character_Dead;
	FGameplayTag State_Character_CanAct;
	FGameplayTag State_Character_MoveConsumed;
	//~ End of CharacterState

	//~ Begin StateTree Event
	FGameplayTag Event_StateTree_PlanPhaseStarted;
	FGameplayTag Event_StateTree_TelegraphPlan;
	//~ End of StateTree Event

	//~ Begin Montage Event
	FGameplayTag Event_Montage_ApplyEffect;
	FGameplayTag Event_Montage_EndUseCard;
	//~ End of Montage Event

private:
	static FLetheGameplayTags GameplayTags;
};
