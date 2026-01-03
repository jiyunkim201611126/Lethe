// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameplayTags.h"
#include "GameplayTagsManager.h"

FLetheGameplayTags FLetheGameplayTags::GameplayTags;

void FLetheGameplayTags::InitializeNativeGameplayTags()
{
	GameplayTags.Attributes_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Health"), FString(""));
	GameplayTags.Attributes_Vital_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxHealth"), FString(""));
	
	GameplayTags.Damage_Normal = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage.Normal"), FString(""));

	GameplayTags.DamageTypeTags.Emplace(GameplayTags.Damage_Normal);
	
	GameplayTags.Abilities_Types_Active = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Types.Active"), FString(""));
	GameplayTags.Abilities_Types_Passive = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Abilities.Types.Passive"), FString(""));
	
	GameplayTags.CharacterState_Dead = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CharacterState.Dead"), FString(""));
	GameplayTags.CharacterState_Knockback = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("CharacterState.Knockback"), FString(""));
}
