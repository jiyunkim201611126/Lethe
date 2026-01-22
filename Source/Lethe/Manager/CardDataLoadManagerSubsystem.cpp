// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardDataLoadManagerSubsystem.h"

#include "Engine/AssetManager.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardOwnerViewData.h"
#include "Lethe/Data/CardSelfViewData.h"

void UCardDataLoadManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const UAssetManager& AssetManager = UAssetManager::Get();
	
	// CardDefinition 타입의 모든 에셋 메타데이터를 가져옵니다. 로드하는 과정이 아닙니다.
	TArray<FAssetData> CardDefinitionAssetDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardDefinition")), CardDefinitionAssetDataList);
	for (const FAssetData& CardDefinitionAssetData : CardDefinitionAssetDataList)
	{
		// 찾은 메타데이터에서 CardTag를 가져와 PrimaryAssetId와 매핑합니다.
		FString FoundCardTagString;
		if (CardDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardDefinitionData, CardTag), FoundCardTagString))
		{
			FGameplayTag CardTag;
			CardTag.FromExportString(FoundCardTagString);
			FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForData(CardDefinitionAssetData);

			CardDefinitionDataAssetIds.Emplace(CardTag, AssetId);
		}
	}

	TArray<FAssetData> CardSelfViewAssetDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardSelfView")), CardSelfViewAssetDataList);
	for (const FAssetData& CardSelfViewAssetData : CardSelfViewAssetDataList)
	{
		FString FoundCardTagString;
		if (CardSelfViewAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardSelfViewData, CardTag), FoundCardTagString))
		{
			FGameplayTag CardTag;
			CardTag.FromExportString(FoundCardTagString);
			FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForData(CardSelfViewAssetData);

			CardSelfViewDataAssetIds.Emplace(CardTag, AssetId);
		}
	}

	TArray<FAssetData> CardOwnerViewAssetDataList;
	AssetManager.GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardOwnerView")), CardOwnerViewAssetDataList);
	for (const FAssetData& CardOwnerViewAssetData : CardOwnerViewAssetDataList)
	{
		FString FoundCharacterTagString;
		if (CardOwnerViewAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardOwnerViewData, CharacterTag), FoundCharacterTagString))
		{
			FGameplayTag CharacterTag;
			CharacterTag.FromExportString(FoundCharacterTagString);
			FPrimaryAssetId AssetId = AssetManager.GetPrimaryAssetIdForData(CardOwnerViewAssetData);

			CardOwnerViewDataAssetIds.Emplace(CharacterTag, AssetId);
		}
	}
}

void UCardDataLoadManagerSubsystem::LoadCardDefinitionData(const TArray<FGameplayTag>& InCardTags, FOnCardDefinitionsLoaded OnComplete)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	// 매개변수로 받은 CardTag로 로드할 PrimaryDataAsset의 Id를 가져옵니다.
	for (const FGameplayTag& CardTag : InCardTags)
	{
		if (CardDefinitionDataAssetIds.Contains(CardTag))
		{
			AssetsToLoad.Emplace(CardDefinitionDataAssetIds[CardTag]);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("CardTag: %s에 해당하는 CardDefinition DataAsset이 없습니다."), *CardTag.GetTagName().ToString());
		}
	}
	
	if (!AssetsToLoad.IsEmpty())
	{
		// 로드할 객체가 있다면 로드를 시작합니다.
		AssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateLambda([this, AssetsToLoad, OnComplete]()
		{
			OnCardDefinitionDataLoaded(AssetsToLoad, OnComplete);
		}));
	}
}

void UCardDataLoadManagerSubsystem::OnCardDefinitionDataLoaded(const TArray<FPrimaryAssetId>& AssetsToLoad, const FOnCardDefinitionsLoaded& OnComplete) const
{
	const UAssetManager& AssetManager = UAssetManager::Get();
			
	TArray<UCardDefinitionData*> LoadedAssets;

	for (const FPrimaryAssetId& Id : AssetsToLoad)
	{
		// 로드가 완료되면 해당 객체를 실제로 가져옵니다.
		if (UCardDefinitionData* LoadedAsset = Cast<UCardDefinitionData>(AssetManager.GetPrimaryAssetObject(Id)))
		{
			LoadedAssets.Emplace(LoadedAsset);
		}
	}

	// 로드된 객체를 콜백으로 반환합니다.
	OnComplete.ExecuteIfBound(LoadedAssets);
}

void UCardDataLoadManagerSubsystem::LoadCardViewData(const FGameplayTag& InCardTag, const FGameplayTag& InCharacterTag, FOnCardViewLoaded OnComplete)
{
	UAssetManager& AssetManager = UAssetManager::Get();

	TArray<FPrimaryAssetId> AssetsToLoad;

	FPrimaryAssetId SelfViewId = CardSelfViewDataAssetIds.Contains(InCardTag) ? CardSelfViewDataAssetIds[InCardTag] : FPrimaryAssetId();
	FPrimaryAssetId OwnerViewId = CardOwnerViewDataAssetIds.Contains(InCharacterTag) ? CardOwnerViewDataAssetIds[InCharacterTag] : FPrimaryAssetId();

	if (SelfViewId.IsValid() && OwnerViewId.IsValid())
	{
		AssetsToLoad.Emplace(SelfViewId);
		AssetsToLoad.Emplace(OwnerViewId);
		
		AssetManager.LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateLambda([this, SelfViewId, OwnerViewId, OnComplete]()
		{
			OnCardViewDataLoaded(SelfViewId, OwnerViewId, OnComplete);
		}));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("CardTag: %s, 혹은 CharacterTag: %s에 해당하는 CardViewData DataAsset이 없습니다."), *InCardTag.GetTagName().ToString(), *InCharacterTag.GetTagName().ToString());
	}
}

void UCardDataLoadManagerSubsystem::OnCardViewDataLoaded(const FPrimaryAssetId& SelfViewId, const FPrimaryAssetId& OwnerViewId, const FOnCardViewLoaded& OnComplete) const
{
	const UAssetManager& AssetManager = UAssetManager::Get();
	
	UCardSelfViewData* SelfViewData = Cast<UCardSelfViewData>(AssetManager.GetPrimaryAssetObject(SelfViewId));
	const UCardOwnerViewData* OwnerViewData = Cast<UCardOwnerViewData>(AssetManager.GetPrimaryAssetObject(OwnerViewId));
	
	OnComplete.ExecuteIfBound(SelfViewData, OwnerViewData);
}
