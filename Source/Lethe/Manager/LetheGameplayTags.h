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
	FGameplayTag Attribute_Vital_Health;
	FGameplayTag Attribute_Vital_MaxHealth;
	FGameplayTag Attribute_Vital_MoveRange;
	FGameplayTag Attribute_Vital_MaxMoveRange;
	FGameplayTag Attribute_Vital_MoveRangeRecovery;
	
	FGameplayTag Attribute_Meta_IncomingDamage;
	
	FGameplayTag Attribute_Vital_Mana;
	FGameplayTag Attribute_Vital_MaxMana;
	FGameplayTag Attribute_Vital_Cost;
	FGameplayTag Attribute_Vital_MaxCost;
	FGameplayTag Attribute_Vital_ManaRecovery;
	FGameplayTag Attribute_Vital_CostRecovery;
	FGameplayTag Attribute_Vital_VisionRange;
	//~ End of Attributes
	
	//~ Begin Damage Types
	FGameplayTag Damage;
	FGameplayTag Damage_Physical_Thrust;

	TArray<FGameplayTag> DamageTypeTags;
	//~ End of Damage Types

	//~ Begin Ability
	FGameplayTag Ability_Move;
	FGameplayTag Ability_Swap;
	//~ End of Ability
	
	//~ Begin Card Types
	FGameplayTag Card_Type_Physical;
	FGameplayTag Card_Type_Magic;
	FGameplayTag Card_Type_Util;
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
	FGameplayTag Event_Montage_EndAbility;
	//~ End of Montage Event

	//~ Begin TargetGroup
	FGameplayTag TargetGroup_Primary;
	FGameplayTag TargetGroup_Penetration;
	FGameplayTag TargetGroup_HalfMoon;
	FGameplayTag TargetGroup_Spread;
	//~ End of TargetGroup

	//~ Begin UI Layer
	FGameplayTag UI_Layer_Game;
	FGameplayTag UI_Layer_GameMenu;
	FGameplayTag UI_Layer_Modal;
	//~ End of UI Layer
	
	//~ Begin UIFeature
	FGameplayTag UI_Feature_DeckEditing;
	FGameplayTag UI_Feature_Attribute;
	FGameplayTag UI_Feature_CardPanel;
	//~ End of UIFeature

private:
	static FLetheGameplayTags GameplayTags;
};
