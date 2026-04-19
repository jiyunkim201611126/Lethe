// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "Engine/DataAsset.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Manager/LetheGameplayTags.h"

ULetheAssetManager& ULetheAssetManager::Get()
{
	check(GEngine);
	ULetheAssetManager* LetheAssetManager = CastChecked<ULetheAssetManager>(GEngine->AssetManager);
	return *LetheAssetManager;
}

void ULetheAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	// 전역으로 선언되어있는 GameplayTags 인스턴스를 초기화합니다.
	FLetheGameplayTags::InitializeNativeGameplayTags();

	// GameplayTags 초기화 직후 Attribute와 Tag를 매핑합니다.
	ULetheAttributeSet::InitializeAttributeTagMap();

	// 커스텀 Context를 사용하기 위해 반드시 호출해줘야 하는 함수입니다.
	UAbilitySystemGlobals::Get().InitGlobalData();
}

void ULetheAssetManager::PostInitialAssetScan()
{
	BuildAssetIdCaches();
	
	Super::PostInitialAssetScan();
}

void ULetheAssetManager::LoadPrimaryDataAssets(const TArray<FGameplayTag>& InGameplayTags, FOnPrimaryDataAssetsLoaded OnComplete)
{
	TArray<FPrimaryAssetId> AssetsToLoad;

	for (const FGameplayTag& GameplayTag : InGameplayTags)
	{
		FPrimaryAssetId AssetId;
		if (TryGetAssetId(GameplayTag, AssetId))
		{
			AssetsToLoad.Emplace(AssetId);
		}
		else
		{
			checkf(false, TEXT("GameplayTag: %s에 해당하는 DataAsset을 찾을 수 없습니다."), *GameplayTag.GetTagName().ToString());
		}
	}

	if (!AssetsToLoad.IsEmpty())
	{
		// 로드할 객체가 있다면 로드를 시작합니다.
		LoadPrimaryAssets(AssetsToLoad, TArray<FName>{}, FStreamableDelegate::CreateWeakLambda(this, [this, AssetsToLoad, OnComplete]()
		{
			OnPrimaryDataAssetsLoaded(AssetsToLoad, OnComplete);
		}));
	}
	else
	{
		OnComplete.ExecuteIfBound(TArray<UPrimaryDataAsset*>());
	}
}

void ULetheAssetManager::OnPrimaryDataAssetsLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnPrimaryDataAssetsLoaded& OnComplete) const
{
	TArray<UPrimaryDataAsset*> LoadedAssets;

	for (const FPrimaryAssetId& Id : LoadedAssetsId)
	{
		// 로드가 완료되면 해당 객체를 실제로 가져옵니다.
		UPrimaryDataAsset* LoadedAsset = CastChecked<UPrimaryDataAsset>(GetPrimaryAssetObject(Id));
		LoadedAssets.Emplace(LoadedAsset);
	}

	// 로드된 객체를 콜백으로 반환합니다.
	OnComplete.ExecuteIfBound(LoadedAssets);
}

bool ULetheAssetManager::TryGetAssetId(const FGameplayTag& GameplayTag, FPrimaryAssetId& OutAssetId) const
{
	if (const FPrimaryAssetId* FoundAssetId = TagToAssetId.Find(GameplayTag))
	{
		OutAssetId = *FoundAssetId;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCardTagById(const uint64 CardId, FGameplayTag& OutCardTag) const
{
	if (const FGameplayTag* FoundCardTag = CardIdToTag.Find(CardId))
	{
		OutCardTag = *FoundCardTag;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCardIdByTag(const FGameplayTag& CardTag, uint64& OutCardId) const
{
	if (const uint64* FoundCardId = CardTagToId.Find(CardTag))
	{
		OutCardId = *FoundCardId;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCharacterTagById(const uint64 CharacterId, FGameplayTag& OutCharacterTag) const
{
	if (const FGameplayTag* FoundCharacterTag = CharacterIdToTag.Find(CharacterId))
	{
		OutCharacterTag = *FoundCharacterTag;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCharacterIdByTag(const FGameplayTag& CharacterTag, uint64& OutCharacterId) const
{
	if (const uint64* FoundCharacterId = CharacterTagToId.Find(CharacterTag))
	{
		OutCharacterId = *FoundCharacterId;
		return true;
	}
	return false;
}

void ULetheAssetManager::BuildAssetIdCaches()
{
	TagToAssetId.Empty();
	CardIdToTag.Empty();
	CardTagToId.Empty();
	CharacterIdToTag.Empty();
	CharacterTagToId.Empty();

	BuildCardDefinitionCache();
	BuildCharacterDefinitionCache();
}

void ULetheAssetManager::BuildCardDefinitionCache()
{
	// CardDefinition 타입의 모든 에셋 메타데이터를 가져옵니다. 로드하는 과정이 아닙니다.
	TArray<FAssetData> CardDefinitionAssetDataList;
	GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardDefinition")), CardDefinitionAssetDataList);
	for (const FAssetData& CardDefinitionAssetData : CardDefinitionAssetDataList)
	{
		// 찾은 메타데이터에서 CardId를 가져와 PrimaryAssetId와 매핑합니다.
		FString FoundCardIdString;
		FString FoundCardTagString;
		const bool bHasCardId = CardDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardDefinitionData, CardId), FoundCardIdString);
		const bool bHasCardTag = CardDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardDefinitionData, CardTag), FoundCardTagString);

		if (!bHasCardId || !bHasCardTag)
		{
			continue;
		}

		uint64 CardId = 0;
		if (!LexTryParseString(CardId, *FoundCardIdString))
		{
			continue;
		}
		
		FGameplayTag CardTag;
		CardTag.FromExportString(FoundCardTagString);

		if (!ensureAlwaysMsgf(CardTag.IsValid(), TEXT("CardTag 메타데이터가 유효하지 않은 CardDefinition Data Asset이 존재합니다. Asset: %s"), *CardDefinitionAssetData.AssetName.ToString()))
		{
			continue;
		}
		ensureAlwaysMsgf(!TagToAssetId.Contains(CardTag), TEXT("GameplayTag: %s가 중복인 PrimaryDataAsset이 존재합니다."), *CardTag.ToString());
		ensureAlwaysMsgf(!CardIdToTag.Contains(CardId), TEXT("CardId: %llu가 중복인 CardDefinition Data Asset이 존재합니다."), CardId);
		ensureAlwaysMsgf(!CardTagToId.Contains(CardTag), TEXT("CardTag: %s가 중복인 CardDefinition Data Asset이 존재합니다."), *CardTag.ToString());
		
		FPrimaryAssetId AssetId = GetPrimaryAssetIdForData(CardDefinitionAssetData);
		
		TagToAssetId.Emplace(CardTag, AssetId);
		CardIdToTag.Emplace(CardId, CardTag);
		CardTagToId.Emplace(CardTag, CardId);
	}
}

void ULetheAssetManager::BuildCharacterDefinitionCache()
{
	TArray<FAssetData> CharacterDefinitionAssetDataList;
	GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CharacterDefinition")), CharacterDefinitionAssetDataList);
	for (const FAssetData& CharacterDefinitionAssetData : CharacterDefinitionAssetDataList)
	{
		FString FoundCharacterIdString;
		FString FoundCharacterTagString;
		const bool bHasCharacterId = CharacterDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCharacterDefinitionData, CharacterId), FoundCharacterIdString);
		const bool bHasCharacterTag = CharacterDefinitionAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCharacterDefinitionData, CharacterTag), FoundCharacterTagString);

		if (!bHasCharacterId || !bHasCharacterTag)
		{
			continue;
		}

		uint64 CharacterId = 0;
		if (!LexTryParseString(CharacterId, *FoundCharacterIdString))
		{
			continue;
		}

		FGameplayTag CharacterTag;
		CharacterTag.FromExportString(FoundCharacterTagString);

		if (!ensureAlwaysMsgf(CharacterTag.IsValid(), TEXT("CharacterTag 메타데이터가 유효하지 않은 CharacterDefinition Data Asset이 존재합니다. Asset: %s"), *CharacterDefinitionAssetData.AssetName.ToString()))
		{
			continue;
		}
		ensureAlwaysMsgf(!TagToAssetId.Contains(CharacterTag), TEXT("GameplayTag: %s가 중복인 PrimaryDataAsset이 존재합니다."), *CharacterTag.ToString());
		ensureAlwaysMsgf(!CharacterIdToTag.Contains(CharacterId), TEXT("CharacterId: %llu가 중복인 CharacterDefinition Data Asset이 존재합니다."), CharacterId);
		ensureAlwaysMsgf(!CharacterTagToId.Contains(CharacterTag), TEXT("CharacterTag: %s가 중복인 CharacterDefinition Data Asset이 존재합니다."), *CharacterTag.ToString());

		FPrimaryAssetId AssetId = GetPrimaryAssetIdForData(CharacterDefinitionAssetData);

		TagToAssetId.Emplace(CharacterTag, AssetId);
		CharacterIdToTag.Emplace(CharacterId, CharacterTag);
		CharacterTagToId.Emplace(CharacterTag, CharacterId);
	}
}

#if WITH_EDITOR
void ULetheAssetManager::PostInitProperties()
{
	Super::PostInitProperties();

	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		OnAssetUpdateOnDiskHandle = AssetRegistry.OnAssetUpdatedOnDisk().AddUObject(this, &ThisClass::OnAssetUpdatedOnDisk);
	}
}

void ULetheAssetManager::BeginDestroy()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetRegistry"))
	{
		IAssetRegistry& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
		AssetRegistry.OnAssetUpdatedOnDisk().Remove(OnAssetUpdateOnDiskHandle);
	}
	
	Super::BeginDestroy();
}

void ULetheAssetManager::OnAssetUpdatedOnDisk(const FAssetData& AssetData)
{
	if (AssetData.AssetClassPath == UCardDefinitionData::StaticClass()->GetClassPathName() ||
		AssetData.AssetClassPath == UCharacterDefinitionData::StaticClass()->GetClassPathName())
	{
		BuildAssetIdCaches();
	}
}
#endif
