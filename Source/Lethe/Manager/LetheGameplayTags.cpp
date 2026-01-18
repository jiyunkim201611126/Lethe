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
	
	GameplayTags.Attributes_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Health"), FString(""));
	GameplayTags.Attributes_Vital_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxHealth"), FString(""));
	
	GameplayTags.Damage_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Physical"), FString(""));

	GameplayTags.DamageTypeTags.Emplace(GameplayTags.Damage_Physical);
	
	GameplayTags.Card_Test = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Test"), FString(""));
	
	GameplayTags.Card_Types_Physical = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Physical"), FString(""));
	GameplayTags.Card_Types_Magic = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Card.Types.Magic"), FString(""));
	
	GameplayTags.CharacterState_Dead = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CharacterState.Dead"), FString(""));
	GameplayTags.CharacterState_Knockback = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CharacterState.Knockback"), FString(""));
}
