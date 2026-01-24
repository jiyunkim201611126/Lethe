// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemComponent.h"

#include "Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardSelfViewData.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
#include "Lethe/SaveGame/DeckSaveGame.h"

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
	if (UDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>())
	{
		// 로드 완료 시 호출될 콜백을 생성합니다.
		const FOnCardDefinitionsLoaded OnLoadComplete = FOnCardDefinitionsLoaded::CreateWeakLambda(this, [this](const TArray<UCardDefinitionData*>& CardDefinitionDatas)
		{
			for (UCardDefinitionData* CardDefinition : CardDefinitionDatas)
			{
				if (CardDefinition && CardDefinition->AbilityClass)
				{
					// Ability 부여 시 SourceObject에 CardDefinitionData를 넣어줍니다.
					FGameplayAbilitySpec Spec(CardDefinition->AbilityClass, 1, INDEX_NONE, CardDefinition);
					GiveAbility(Spec);
				}
			}
		});

		// CardDefinitionDataAsset 로드를 시작합니다.
		TArray<FGameplayTag> CardTags;
		for (const FSavedCard& SavedCard : InSavedCards)
		{
			CardTags.Emplace(SavedCard.CardTag);
		}
		
		CardDataLoadManagerSubsystem->LoadCardDefinitionData(CardTags, OnLoadComplete);
	}
}

void ULetheAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	
	UDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>();
	const UCardDefinitionData* CardDefinitionData = Cast<UCardDefinitionData>(AbilitySpec.SourceObject);
	IPlayableCharacterInterface* PlayerCharacter = Cast<IPlayableCharacterInterface>(GetOwner());
	
	if (CardDataLoadManagerSubsystem && CardDefinitionData && PlayerCharacter)
	{
		const FOnCardViewLoaded OnLoadComplete = FOnCardViewLoaded::CreateWeakLambda(this, [this, CardDefinitionData](const UCardSelfViewData* SelfViewData, const UCharacterDefinitionData* OwnerViewData)
		{
			if (!CardDefinitionData)
			{
				return;
			}
			
			OnCardViewDataLoadFinished(CardDefinitionData, SelfViewData, OwnerViewData);
		});

		// CardSelfView Data Asset, CharacterDefinition Data Asset 로드를 시작합니다.
		CardDataLoadManagerSubsystem->LoadCardViewData(CardDefinitionData->CardTag, PlayerCharacter->GetCharacterTag(), OnLoadComplete);
	}
}

void ULetheAbilitySystemComponent::OnCardViewDataLoadFinished(const UCardDefinitionData* CardDefinitionData, const UCardSelfViewData* CardSelfViewData, const UCharacterDefinitionData* CharacterDefinitionData)
{
	// 카드 위젯이 생성될 수 있도록 콜백을 호출합니다.
	OnAbilityGivenDelegate.ExecuteIfBound(this, CardDefinitionData, CardSelfViewData, CharacterDefinitionData);
}
