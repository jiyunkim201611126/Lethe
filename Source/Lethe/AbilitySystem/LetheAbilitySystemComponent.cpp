// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAbilitySystemComponent.h"

#include "Abilities/LetheGameplayAbility.h"
#include "Engine/AssetManager.h"
#include "Lethe/Data/CardDataLoader.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardSelfViewData.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"

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
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;
	AssetsToLoad.Reserve(InCardTags.Num());

	for (const FGameplayTag& CardTag : InCardTags)
	{
		if (!CardTag.IsValid())
		{
			continue;
		}

		FPrimaryAssetId AssetId(TEXT("CardDefinition"), CardTag.GetTagName());
		AssetsToLoad.Add(AssetId);
	}

	TWeakObjectPtr<ULetheAbilitySystemComponent> WeakThis(this);

	AssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateLambda([WeakThis, AssetsToLoad]
	{
		if (!WeakThis.IsValid())
		{
			return;
		}

		for (const FPrimaryAssetId& AssetId : AssetsToLoad)
		{
			UCardDefinitionData* CardDefinition = Cast<UCardDefinitionData>(UAssetManager::Get().GetPrimaryAssetObject(AssetId));
			if (!CardDefinition || !CardDefinition->AbilityClass)
			{
				continue;
			}

			// AbilitySpec을 생성할 때 SourceObject로 CardDefinitionData를 넣어줍니다.
			FGameplayAbilitySpec Spec(CardDefinition->AbilityClass, 1, INDEX_NONE, CardDefinition);

			WeakThis->GiveAbility(Spec);
		}
	}));
}

void ULetheAbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);

	if (const UCardDefinitionData* CardDefinitionData = Cast<UCardDefinitionData>(AbilitySpec.SourceObject))
	{
		if (const IPlayableCharacterInterface* PlayerCharacter = Cast<IPlayableCharacterInterface>(GetOwner()))
		{
			if (UCardDataLoader* Loader = NewObject<UCardDataLoader>(this))
			{
				Loader->OnLoadFinishedDelegate.BindUObject(this, &ThisClass::OnCardViewDataLoadFinished);
				Loader->Init(PlayerCharacter->GetCharacterTag(), CardDefinitionData, Cast<ULetheGameplayAbility>(AbilitySpec.Ability));
			}
		}
	}
}

void ULetheAbilitySystemComponent::OnCardViewDataLoadFinished(const ULetheGameplayAbility* Ability, const UCardDefinitionData* CardDefinition, UCardSelfViewData* CardSelfViewData, const UCardOwnerViewData* CardOwnerViewData)
{
	// Ability에서 CardDescription을 가져와 DataAsset에 넣어줍니다.
	if (Ability && CardSelfViewData && CardSelfViewData->CardDescriptionText.IsEmpty())
	{
		CardSelfViewData->CardDescriptionText = Ability->GetCardDescription(Ability->GetAbilityLevel());
	}
	
	OnAbilityGivenDelegate.ExecuteIfBound(this, CardDefinition, CardSelfViewData, CardOwnerViewData);
}
