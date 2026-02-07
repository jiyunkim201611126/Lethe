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

	GameplayTags.Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage"), FString(""));
	GameplayTags.Damage_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Physical"), FString(""));

	GameplayTags.DamageTypeTags.Emplace(GameplayTags.Damage_Physical);
	
	GameplayTags.Card_Ability_PhysicalTest = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Ability.PhysicalTest"), FString(""));
	GameplayTags.Card_Ability_MagicTest = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Ability.MagicTest"), FString(""));
	GameplayTags.Card_Ability_UtilTest = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Ability.UtilTest"), FString(""));
	GameplayTags.Card_Ability_JustTest = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Ability.JustTest"), FString(""));
	
	GameplayTags.Card_Types_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Physical"), FString(""));
	GameplayTags.Card_Types_Magic = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Magic"), FString(""));
	GameplayTags.Card_Types_Util = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Util"), FString(""));
	
	GameplayTags.CharacterState_Dead = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CharacterState.Dead"), FString(""));
	GameplayTags.CharacterState_Knockback = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CharacterState.Knockback"), FString(""));
}
