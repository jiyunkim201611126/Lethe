// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeHelper.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "PlayerAttributeSet.generated.h"

UCLASS(NotBlueprintable)
class LETHE_API UPlayerAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	static void InitializePlayerAttributeTagMap();
	
	//~ Begin UAttributeSet Interface
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	//~ End of UAttributeSet Interface

public:
	/** Attribute와 그에 해당하는 Tag를 매핑한 TMap입니다. */
	static TMap<FGameplayAttribute, FGameplayTag> PlayerAttributesToTags;
	
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Vital Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Mana);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxMana);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData Cost;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Cost);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxCost;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxCost);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData ManaRecovery;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, ManaRecovery);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData CostRecovery;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, CostRecovery);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData VisionRange;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, VisionRange);
};
