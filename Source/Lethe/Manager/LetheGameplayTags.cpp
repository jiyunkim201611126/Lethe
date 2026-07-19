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
	
	GameplayTags.Attribute_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.Health"), FString(""));
	GameplayTags.Attribute_Vital_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.MaxHealth"), FString(""));
	GameplayTags.Attribute_Vital_MoveRange = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.MoveRange"), FString(""));
	GameplayTags.Attribute_Vital_MaxMoveRange = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.MaxMoveRange"), FString(""));
	GameplayTags.Attribute_Vital_MoveRangeRecovery = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.MoveRangeRecovery"), FString(""));
	
	GameplayTags.Attribute_Meta_IncomingDamage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Meta.IncomingDamage"), FString(""));
	
	GameplayTags.Attribute_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.Mana"), FString(""));
	GameplayTags.Attribute_Vital_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.MaxMana"), FString(""));
	GameplayTags.Attribute_Vital_Cost = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.Cost"), FString(""));
	GameplayTags.Attribute_Vital_MaxCost = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.MaxCost"), FString(""));
	GameplayTags.Attribute_Vital_ManaRecovery = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.ManaRecovery"), FString(""));
	GameplayTags.Attribute_Vital_CostRecovery = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.CostRecovery"), FString(""));
	GameplayTags.Attribute_Vital_VisionRange = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attribute.Vital.VisionRange"), FString(""));

	GameplayTags.Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage"), FString(""));
	GameplayTags.Damage_Physical_Thrust = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Physical.Thrust"), FString(""));

	GameplayTags.DamageTypeTags.Emplace(GameplayTags.Damage_Physical_Thrust);
	
	GameplayTags.Ability_Move = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Move"), FString(""));
	GameplayTags.Ability_Swap = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Ability.Swap"), FString(""));
	
	GameplayTags.Card_Type_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Type.Physical"), FString(""));
	GameplayTags.Card_Type_Magic = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Type.Magic"), FString(""));
	GameplayTags.Card_Type_Util = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Type.Util"), FString(""));
	
	GameplayTags.State_Character_Dead = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("State.Character.Dead"), FString(""));
	GameplayTags.State_Character_CanAct = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("State.Character.CanAct"), FString(""));
	GameplayTags.State_Character_MoveConsumed = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("State.Character.MoveConsumed"), FString(""));
	
	GameplayTags.Event_StateTree_PlanPhaseStarted = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.StateTree.PlanPhaseStarted"), FString(""));
	GameplayTags.Event_StateTree_TelegraphPlan = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.StateTree.TelegraphPlan"), FString(""));
	
	GameplayTags.Event_Montage_EndAbility = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Event.Montage.EndAbility"), FString(""));
	
	GameplayTags.TargetTileGroup_Primary = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("TargetTileGroup.Primary"), FString(""));
	GameplayTags.TargetTileGroup_Penetration = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("TargetTileGroup.Penetration"), FString(""));
	GameplayTags.TargetTileGroup_HalfMoon = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("TargetTileGroup.HalfMoon"), FString(""));
	GameplayTags.TargetTileGroup_Spread = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("TargetTileGroup.Spread"), FString(""));
	
	GameplayTags.UI_Layer_Game = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Layer.Game"), FString(""));
	GameplayTags.UI_Layer_GameMenu = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Layer.GameMenu"), FString(""));
	GameplayTags.UI_Layer_Modal = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Layer.Modal"), FString(""));
	
	GameplayTags.UI_Feature_DeckEditing = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Feature.DeckEditing"), FString(""));
	GameplayTags.UI_Feature_Attribute = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Feature.Attribute"), FString(""));
	GameplayTags.UI_Feature_CardPanel = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("UI.Feature.CardPanel"), FString(""));
}
