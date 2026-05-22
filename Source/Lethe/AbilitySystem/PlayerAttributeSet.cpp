// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerAttributeSet.h"

#include "GameplayEffectExtension.h"
#include "Lethe/Manager/LetheGameplayTags.h"

TMap<FGameplayAttribute, FGameplayTag> UPlayerAttributeSet::PlayerAttributesToTags;

void UPlayerAttributeSet::InitializePlayerAttributeTagMap()
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (PlayerAttributesToTags.IsEmpty())
	{
		PlayerAttributesToTags.Reserve(6);

		PlayerAttributesToTags.Add(GetManaAttribute(), LetheGameplayTags.Attribute_Vital_Mana);
		PlayerAttributesToTags.Add(GetMaxManaAttribute(), LetheGameplayTags.Attribute_Vital_MaxMana);
		PlayerAttributesToTags.Add(GetCostAttribute(), LetheGameplayTags.Attribute_Vital_Cost);
		PlayerAttributesToTags.Add(GetMaxCostAttribute(), LetheGameplayTags.Attribute_Vital_MaxCost);
		PlayerAttributesToTags.Add(GetManaRecoveryAttribute(), LetheGameplayTags.Attribute_Vital_ManaRecovery);
		PlayerAttributesToTags.Add(GetCostRecoveryAttribute(), LetheGameplayTags.Attribute_Vital_CostRecovery);
	}
}

void UPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	if (Data.EvaluatedData.Attribute == GetCostAttribute())
	{
		SetCost(FMath::Clamp(GetCost(), 0.f, GetMaxCost()));
	}
}
