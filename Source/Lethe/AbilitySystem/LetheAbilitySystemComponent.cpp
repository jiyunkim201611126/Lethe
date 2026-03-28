// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemComponent.h"

#include "Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardPrimaryDataAssetLoader.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"

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
	if (IPlayerCharacterInterface* PlayerCharacter = Cast<IPlayerCharacterInterface>(GetOwner()))
	{
		UDataLoadManagerSubsystem* DataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>();
		UCardPrimaryDataAssetLoader* Loader = UCardPrimaryDataAssetLoader::CreateLoader(this);
		if (DataLoadManagerSubsystem && Loader)
		{
			DataLoadManagerSubsystem->AddLoader(Loader);
			const FOnAllCardDataLoaded OnLoadedCallback = FOnAllCardDataLoaded::CreateUObject(this, &ThisClass::OnAllCardsLoaded);
			Loader->LoadCardData(PlayerCharacter->GetCharacterTag(), InSavedCards, true, OnLoadedCallback);
		}
	}
}

void ULetheAbilitySystemComponent::OnAllCardsLoaded(const FGameplayTag& CharacterTag, const TArray<FLoadedCardInfo>& LoadedCards, bool bEquipped)
{
	for (const FLoadedCardInfo& CardInfo : LoadedCards)
	{
		if (CardInfo.CardDefinition && CardInfo.CardDefinition->AbilityClass)
		{
			// Ability 부여 시 SourceObject에 CardDefinitionData를 넣어줍니다.
			FGameplayAbilitySpec Spec(CardInfo.CardDefinition->AbilityClass, CardInfo.SavedCardInfo.CardLevel, INDEX_NONE, CardInfo.CardDefinition);
			GiveAbility(Spec);

			// 카드 위젯이 생성될 수 있도록 콜백을 호출합니다.
			OnAbilityGivenDelegate.ExecuteIfBound(this, CardInfo.CardDefinition, CardInfo.SelfViewData, CardInfo.CharacterDefinition);
		}
	}
}
