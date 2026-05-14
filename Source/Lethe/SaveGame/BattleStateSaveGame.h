// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "BattleStateSaveGame.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;

USTRUCT(BlueprintType)
struct FSavedAttributeValue
{
	GENERATED_BODY()

	UPROPERTY()
	FName AttributeSetName;

	UPROPERTY()
	FName AttributeName;

	UPROPERTY()
	float BaseValue = 0.f;
};

USTRUCT(BlueprintType)
struct FSavedBattleCharacterState
{
	GENERATED_BODY()

	UPROPERTY()
	int64 CharacterId;

	UPROPERTY()
	TArray<FSavedAttributeValue> Attributes;
};

/**
 * 현재 플레이 상태를 저장하는 SaveGame입니다. 
 */
UCLASS()
class LETHE_API UBattleStateSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	void SavePlayerCharacterAttributes(const int64 CharacterId, const UAbilitySystemComponent* AbilitySystemComponent);

	/** 해당 캐릭터에게 저장되어 있던 State를 적용합니다. */
	bool ApplyPlayerCharacterAttributes(const int64 CharacterId, UAbilitySystemComponent* AbilitySystemComponent) const;

private:
	/** Property가 SaveGame이 붙은 Attribute인지 확인합니다. */
	bool IsSaveGameAttributeProperty(const FProperty* Property) const;
	const UAttributeSet* FindAttributeSetByName(const UAbilitySystemComponent* AbilitySystemComponent, const FName AttributeSetName) const;
	const FSavedBattleCharacterState* FindPlayerCharacterState(const int64 CharacterId) const;
	
	void CaptureSaveGameAttributes(const UAbilitySystemComponent* AbilitySystemComponent, TArray<FSavedAttributeValue>& OutAttributes) const;
	bool ApplySaveGameAttributes(UAbilitySystemComponent* AbilitySystemComponent, const TArray<FSavedAttributeValue>& Attributes) const;

public:
	UPROPERTY()
	TArray<FSavedBattleCharacterState> PlayerCharacters;
};
