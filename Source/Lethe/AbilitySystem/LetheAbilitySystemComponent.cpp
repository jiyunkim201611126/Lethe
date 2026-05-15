// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemComponent.h"

#include "Ability/LetheGameplayAbility.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Manager/CardDataLoadSubsystem.h"

void ULetheAbilitySystemComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities)
{
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : InAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		GiveAbility(AbilitySpec);
	}
}

void ULetheAbilitySystemComponent::AddCharacterAbilitiesWithActive(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities)
{
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : InAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		GiveAbilityAndActivateOnce(AbilitySpec);
	}
}

void ULetheAbilitySystemComponent::AddCharacterAbilities(const TArray<FSavedCard>& InSavedCards)
{
	if (ALetheCharacterBase* OwnerCharacter = Cast<ALetheCharacterBase>(GetOwner()))
	{
		UCardDataLoadSubsystem* CardDataLoadSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCardDataLoadSubsystem>();
		if (CardDataLoadSubsystem)
		{
			const FOnAllCardDataLoaded OnLoadedCallback = FOnAllCardDataLoaded::CreateUObject(this, &ThisClass::OnAllCardsLoaded);
			CardDataLoadSubsystem->LoadCardData(OwnerCharacter->GetCharacterTag(), InSavedCards, true, OnLoadedCallback);
		}
	}
}

void ULetheAbilitySystemComponent::OnAllCardsLoaded(const FGameplayTag& CharacterTag, const FLoadedCardInfos& LoadedCardInfos, bool bEquipped)
{
	for (int32 CardIndex = 0; CardIndex < LoadedCardInfos.CardDefinitions.Num(); ++CardIndex)
	{
		if (LoadedCardInfos.CardDefinitions.IsValidIndex(CardIndex) && LoadedCardInfos.SavedCardInfos.IsValidIndex(CardIndex))
		{
			UCardDefinitionData* CardDefinition = LoadedCardInfos.CardDefinitions[CardIndex];
			if (CardDefinition && CardDefinition->AbilityClass)
			{
				// Ability 부여 시 SourceObject에 CardDefinitionData를 넣어줍니다.
				FGameplayAbilitySpec Spec(CardDefinition->AbilityClass, LoadedCardInfos.SavedCardInfos[CardIndex].CardLevel, INDEX_NONE, CardDefinition);
				GiveAbility(Spec);

				// 카드 위젯이 생성될 수 있도록 콜백을 호출합니다.
				OnAbilityGivenDelegate.ExecuteIfBound(this, CardDefinition, LoadedCardInfos.CharacterDefinition);
			}
		}
	}
}
