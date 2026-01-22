// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardDataLoader.h"

#include "CardDefinitionData.h"
#include "CardOwnerViewData.h"
#include "CardSelfViewData.h"
#include "Engine/AssetManager.h"

void UCardDataLoader::Init(const FGameplayTag& InCharacterTag, const UCardDefinitionData* InCardDefinition, const ULetheGameplayAbility* InAbility)
{
	CharacterTag = InCharacterTag;
	Ability = InAbility;
	CardDefinition = InCardDefinition;

	LoadCardSelfViewData();
	LoadCardOwnerViewData();
}

void UCardDataLoader::LoadCardSelfViewData()
{
	if (!CharacterTag.IsValid() || !CardDefinition)
	{
		return;
	}

	const FPrimaryAssetId AssetId(TEXT("CardSelfView"), CardDefinition->CardTag.GetTagName());

	UAssetManager::Get().LoadPrimaryAsset(AssetId, TArray<FName>{}, FStreamableDelegate::CreateUObject(this, &ThisClass::OnCardSelfViewDataLoaded, AssetId));
}

void UCardDataLoader::LoadCardOwnerViewData()
{
	if (!CharacterTag.IsValid())
	{
		return;
	}

	const FPrimaryAssetId AssetId(TEXT("CardOwnerView"), CharacterTag.GetTagName());

	UAssetManager::Get().LoadPrimaryAsset(AssetId, TArray<FName>{}, FStreamableDelegate::CreateUObject(this, &ThisClass::OnCardOwnerViewDataLoaded, AssetId));
}

void UCardDataLoader::OnCardSelfViewDataLoaded(FPrimaryAssetId AssetId)
{
	CardSelfViewData = Cast<UCardSelfViewData>(UAssetManager::Get().GetPrimaryAssetObject(AssetId));

	TryFinish();
}

void UCardDataLoader::OnCardOwnerViewDataLoaded(FPrimaryAssetId AssetId)
{
	CardOwnerViewData = Cast<UCardOwnerViewData>(UAssetManager::Get().GetPrimaryAssetObject(AssetId));
	
	TryFinish();
}

void UCardDataLoader::TryFinish()
{
	if (!CardDefinition || !CardSelfViewData || !CardOwnerViewData)
	{
		return;
	}

	OnLoadFinishedDelegate.ExecuteIfBound(Ability, CardDefinition, CardSelfViewData, CardOwnerViewData);
	
	CardDefinition = nullptr;
	CardSelfViewData = nullptr;
	CardOwnerViewData = nullptr;
	OnLoadFinishedDelegate.Unbind();

	MarkAsGarbage();
}
