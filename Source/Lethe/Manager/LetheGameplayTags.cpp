// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayTags.h"
#include "GameplayTagsManager.h"

FLetheGameplayTags FLetheGameplayTags::GameplayTags;

void FLetheGameplayTags::InitializeNativeGameplayTags()
{	
	GameplayTags.Character_Test0 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Character.Test0"), FString(""));
	GameplayTags.Character_Test1 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Character.Test1"), FString(""));
	GameplayTags.Character_Test2 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Character.Test2"), FString(""));
	GameplayTags.Character_Test3 = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Character.Test3"), FString(""));
	
	GameplayTags.Attributes_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Health"), FString(""));
	GameplayTags.Attributes_Vital_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxHealth"), FString(""));
	GameplayTags.Attributes_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Mana"), FString(""));
	GameplayTags.Attributes_Vital_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxMana"), FString(""));
	GameplayTags.Attributes_Vital_Cost = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Cost"), FString(""));
	GameplayTags.Attributes_Vital_MaxCost = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxCost"), FString(""));
	GameplayTags.Attributes_Vital_ManaRecovery = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.ManaRecovery"), FString(""));
	GameplayTags.Attributes_Vital_CostRecovery = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.CostRecovery"), FString(""));
	
	GameplayTags.Attributes_Meta_IncomingDamage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Meta.IncomingDamage"), FString(""));

	GameplayTags.Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage"), FString(""));
	GameplayTags.Damage_Physical_Thrust = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Physical.Thrust"), FString(""));

	GameplayTags.DamageTypeTags.Emplace(GameplayTags.Damage_Physical_Thrust);
	
	GameplayTags.Ability_Move = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Move"), FString(""));
	
	GameplayTags.Card_Types_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Physical"), FString(""));
	GameplayTags.Card_Types_Magic = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Magic"), FString(""));
	GameplayTags.Card_Types_Util = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Util"), FString(""));
	
	GameplayTags.State_Character_Dead = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("State.Character.Dead"), FString(""));
	GameplayTags.State_Character_CanAct = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("State.Character.CanAct"), FString(""));
	GameplayTags.State_Character_MoveConsumed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("State.Character.MoveConsumed"), FString(""));
	
	GameplayTags.Event_StateTree_MovePhaseStarted = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.StateTree.MovePhaseStarted"), FString(""));
	GameplayTags.Event_StateTree_DrawPhaseStarted = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.StateTree.DrawPhaseStarted"), FString(""));
	GameplayTags.Event_StateTree_OwnTurnPhaseStarted = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.StateTree.OwnTurnPhaseStarted"), FString(""));
	
	GameplayTags.Event_Montage_ApplyEffect = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.Montage.ApplyEffect"), FString(""));
	GameplayTags.Event_Montage_EndUseCard = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.Montage.EndUseCard"), FString(""));
}
