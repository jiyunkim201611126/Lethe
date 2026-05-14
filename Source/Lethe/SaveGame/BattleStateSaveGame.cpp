// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleStateSaveGame.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "UObject/UnrealType.h"

void UBattleStateSaveGame::SavePlayerCharacterAttributes(const int64 CharacterId, const UAbilitySystemComponent* AbilitySystemComponent)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FSavedBattleCharacterState* CharacterState = PlayerCharacters.FindByPredicate([CharacterId](const FSavedBattleCharacterState& SavedCharacterState)
	{
		return SavedCharacterState.CharacterId == CharacterId;
	});

	if (!CharacterState)
	{
		CharacterState = &PlayerCharacters.AddDefaulted_GetRef();
	}

	CharacterState->CharacterId = CharacterId;
	CaptureSaveGameAttributes(AbilitySystemComponent, CharacterState->Attributes);
}

bool UBattleStateSaveGame::ApplyPlayerCharacterAttributes(const int64 CharacterId, UAbilitySystemComponent* AbilitySystemComponent) const
{
	const FSavedBattleCharacterState* CharacterState = FindPlayerCharacterState(CharacterId);
	if (!CharacterState || !AbilitySystemComponent)
	{
		return false;
	}

	return ApplySaveGameAttributes(AbilitySystemComponent, CharacterState->Attributes);
}

bool UBattleStateSaveGame::IsSaveGameAttributeProperty(const FProperty* Property) const
{
	const FStructProperty* StructProperty = CastField<FStructProperty>(Property);
	return StructProperty && StructProperty->Struct == FGameplayAttributeData::StaticStruct() && Property->HasAnyPropertyFlags(CPF_SaveGame);
}

const UAttributeSet* UBattleStateSaveGame::FindAttributeSetByName(const UAbilitySystemComponent* AbilitySystemComponent, const FName AttributeSetName) const
{
	if (!AbilitySystemComponent || AttributeSetName.IsNone())
	{
		return nullptr;
	}

	for (const UAttributeSet* AttributeSet : AbilitySystemComponent->GetSpawnedAttributes())
	{
		if (AttributeSet && AttributeSet->GetClass()->GetFName() == AttributeSetName)
		{
			return AttributeSet;
		}
	}

	return nullptr;
}

const FSavedBattleCharacterState* UBattleStateSaveGame::FindPlayerCharacterState(const int64 CharacterId) const
{
	return PlayerCharacters.FindByPredicate([CharacterId](const FSavedBattleCharacterState& SavedCharacterState)
	{
		return SavedCharacterState.CharacterId == CharacterId;
	});
}

void UBattleStateSaveGame::CaptureSaveGameAttributes(const UAbilitySystemComponent* AbilitySystemComponent, TArray<FSavedAttributeValue>& OutAttributes) const
{
	OutAttributes.Reset();
	if (!AbilitySystemComponent)
	{
		return;
	}

	// ASC에 붙어있는 모든 AttributeSet을 순회합니다.
	// 현재로선 하나만 붙어있으나 확장 가능성에 대비합니다.
	for (const UAttributeSet* AttributeSet : AbilitySystemComponent->GetSpawnedAttributes())
	{
		if (!AttributeSet)
		{
			continue;
		}

		// AttributeSet 클래스 안에 있는 UPROPERTY Reflection Property Field를 순회합니다.
		for (TFieldIterator<FProperty> PropertyIterator(AttributeSet->GetClass(), EFieldIteratorFlags::IncludeSuper); PropertyIterator; ++PropertyIterator)
		{
			FProperty* Property = *PropertyIterator;
			if (!IsSaveGameAttributeProperty(Property))
			{
				continue;
			}

			// Attribute의 이름과 Value를 가져와 Out 인자에 추가합니다.
			const FGameplayAttribute Attribute(Property);
			FSavedAttributeValue SavedAttribute;
			SavedAttribute.AttributeSetName = AttributeSet->GetClass()->GetFName();
			SavedAttribute.AttributeName = Property->GetFName();
			SavedAttribute.BaseValue = AbilitySystemComponent->GetNumericAttributeBase(Attribute);

			OutAttributes.Add(SavedAttribute);
		}
	}
}

bool UBattleStateSaveGame::ApplySaveGameAttributes(UAbilitySystemComponent* AbilitySystemComponent, const TArray<FSavedAttributeValue>& Attributes) const
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	bool bAppliedAny = false;
	for (const FSavedAttributeValue& SavedAttribute : Attributes)
	{
		const UAttributeSet* AttributeSet = const_cast<UAttributeSet*>(FindAttributeSetByName(AbilitySystemComponent, SavedAttribute.AttributeSetName));
		if (!AttributeSet)
		{
			continue;
		}

		FProperty* Property = FindFProperty<FProperty>(AttributeSet->GetClass(), SavedAttribute.AttributeName);
		if (!IsSaveGameAttributeProperty(Property))
		{
			continue;
		}

		const FGameplayAttribute Attribute(Property);
		AbilitySystemComponent->SetNumericAttributeBase(Attribute, SavedAttribute.BaseValue);

		bAppliedAny = true;
	}

	return bAppliedAny;
}
