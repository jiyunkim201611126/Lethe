// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemComponent.h"

#include "Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardSelfViewData.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/CardDataLoadManagerSubsystem.h"

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

void ULetheAbilitySystemComponent::AddCharacterAbilities(const TArray<FGameplayTag>& InCardTags)
{
	if (UCardDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCardDataLoadManagerSubsystem>())
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
		CardDataLoadManagerSubsystem->LoadCardDefinitionData(InCardTags, OnLoadComplete);
	}
}

void ULetheAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	
	UCardDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCardDataLoadManagerSubsystem>();
	const UCardDefinitionData* CardDefinitionData = Cast<UCardDefinitionData>(AbilitySpec.SourceObject);
	const IPlayableCharacterInterface* PlayerCharacter = Cast<IPlayableCharacterInterface>(GetOwner());
	
	if (CardDataLoadManagerSubsystem && CardDefinitionData && PlayerCharacter)
	{
		const FOnCardViewLoaded OnLoadComplete = FOnCardViewLoaded::CreateWeakLambda(this, [this, CardDefinitionData, AbilitySpec](UCardSelfViewData* SelfViewData, const UCardOwnerViewData* OwnerViewData)
		{
			if (!CardDefinitionData)
			{
				return;
			}
			
			OnCardViewDataLoadFinished(CardDefinitionData, SelfViewData, OwnerViewData, AbilitySpec);
		});

		// CardSelfViewData, CardOwnerViewData Asset 로드를 시작합니다.
		CardDataLoadManagerSubsystem->LoadCardViewData(CardDefinitionData->CardTag, PlayerCharacter->GetCharacterTag(), OnLoadComplete);
	}
}

void ULetheAbilitySystemComponent::OnCardViewDataLoadFinished(const UCardDefinitionData* CardDefinitionData, UCardSelfViewData* CardSelfViewData, const UCardOwnerViewData* CardOwnerViewData, const FGameplayAbilitySpec& AbilitySpec)
{
	// Ability에서 CardDescription을 가져와 DataAsset에 넣어줍니다.
	if (CardSelfViewData)
	{
		const ULetheGameplayAbility* Ability = Cast<ULetheGameplayAbility>(AbilitySpec.Ability);
		CardSelfViewData->CardDescriptionText = Ability->GetCardDescription(Ability->GetAbilityLevel());
	}

	// 카드 위젯이 생성될 수 있도록 콜백을 호출합니다.
	OnAbilityGivenDelegate.ExecuteIfBound(this, CardDefinitionData, CardSelfViewData, CardOwnerViewData);
}
