// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAssetManager.h"

#include "AbilitySystemGlobals.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardSelfViewData.h"
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

bool ULetheAssetManager::TryGetCardDefinitionAssetId(const FGameplayTag& CardTag, FPrimaryAssetId& OutAssetId) const
{
	if (const FPrimaryAssetId* FoundAssetId = CardDefinitionAssetIds.Find(CardTag))
	{
		OutAssetId = *FoundAssetId;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCardSelfViewAssetId(const FGameplayTag& CardTag, FPrimaryAssetId& OutAssetId) const
{
	if (const FPrimaryAssetId* FoundAssetId = CardSelfViewAssetIds.Find(CardTag))
	{
		OutAssetId = *FoundAssetId;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCharacterDefinitionAssetId(const FGameplayTag& CharacterTag, FPrimaryAssetId& OutAssetId) const
{
	if (const FPrimaryAssetId* FoundAssetId = CharacterDefinitionAssetIds.Find(CharacterTag))
	{
		OutAssetId = *FoundAssetId;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCardTagById(uint64 CardId, FGameplayTag& OutCardTag) const
{
	if (const FGameplayTag* FoundCardTag = CardIdToTags.Find(CardId))
	{
		OutCardTag = *FoundCardTag;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCardIdByTag(const FGameplayTag& CardTag, uint64& OutCardId) const
{
	if (const uint64* FoundCardId = CardTagToIds.Find(CardTag))
	{
		OutCardId = *FoundCardId;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCharacterTagById(uint64 CharacterId, FGameplayTag& OutCharacterTag) const
{
	if (const FGameplayTag* FoundCharacterTag = CharacterIdToTags.Find(CharacterId))
	{
		OutCharacterTag = *FoundCharacterTag;
		return true;
	}
	return false;
}

bool ULetheAssetManager::TryGetCharacterIdByTag(const FGameplayTag& CharacterTag, uint64& OutCharacterId) const
{
	if (const uint64* FoundCharacterId = CharacterTagToIds.Find(CharacterTag))
	{
		OutCharacterId = *FoundCharacterId;
		return true;
	}
	return false;
}

void ULetheAssetManager::BuildAssetIdCaches()
{
	BuildCardDefinitionCache();
	BuildCardSelfViewCache();
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

		checkf(CardTag.IsValid(), TEXT("CardTag 메타데이터가 유효하지 않은 CardDefinition Data Asset이 존재합니다. Asset: %s"), *CardDefinitionAssetData.AssetName.ToString());
		checkf(!CardIdToTags.Contains(CardId), TEXT("CardId: %llu가 중복인 CardDefinition Data Asset이 존재합니다."), CardId);
		checkf(!CardTagToIds.Contains(CardTag), TEXT("CardTag: %s가 중복인 CardDefinition Data Asset이 존재합니다."), *CardTag.ToString());
		
		FPrimaryAssetId AssetId = GetPrimaryAssetIdForData(CardDefinitionAssetData);
		
		CardDefinitionAssetIds.Emplace(CardTag, AssetId);
		CardIdToTags.Emplace(CardId, CardTag);
		CardTagToIds.Emplace(CardTag, CardId);
	}
}

void ULetheAssetManager::BuildCardSelfViewCache()
{
	TArray<FAssetData> CardSelfViewAssetDataList;
	GetPrimaryAssetDataList(FPrimaryAssetType(TEXT("CardSelfView")), CardSelfViewAssetDataList);
	for (const FAssetData& CardSelfViewAssetData : CardSelfViewAssetDataList)
	{
		FString FoundCardIdString;
		const bool bHasCardId = CardSelfViewAssetData.GetTagValue(GET_MEMBER_NAME_CHECKED(UCardSelfViewData, CardId), FoundCardIdString);
		if (!bHasCardId)
		{
			continue;
		}

		uint64 CardId = 0;
		if (!LexTryParseString(CardId, *FoundCardIdString))
		{
			continue;
		}

		const FGameplayTag* CardTag = CardIdToTags.Find(CardId);
		checkf(CardTag, TEXT("CardId: %llu가 일치하지 않는 CardDefinition과 CardSelfView Data Asset이 존재합니다."), CardId);
		checkf(!CardSelfViewAssetIds.Contains(*CardTag), TEXT("CardId: %llu가 중복인 CardSelfView Data Asset이 존재합니다."), CardId);

		FPrimaryAssetId AssetId = GetPrimaryAssetIdForData(CardSelfViewAssetData);
		CardSelfViewAssetIds.Emplace(*CardTag, AssetId);
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

		checkf(CharacterTag.IsValid(), TEXT("CharacterTag 메타데이터가 유효하지 않은 CharacterDefinition Data Asset이 존재합니다. Asset: %s"), *CharacterDefinitionAssetData.AssetName.ToString());
		checkf(!CharacterIdToTags.Contains(CharacterId), TEXT("CharacterId: %llu가 중복인 CharacterDefinition Data Asset이 존재합니다."), CharacterId);
		checkf(!CharacterTagToIds.Contains(CharacterTag), TEXT("CharacterTag: %s가 중복인 CharacterDefinition Data Asset이 존재합니다."), *CharacterTag.ToString());

		FPrimaryAssetId AssetId = GetPrimaryAssetIdForData(CharacterDefinitionAssetData);

		CharacterDefinitionAssetIds.Emplace(CharacterTag, AssetId);
		CharacterIdToTags.Emplace(CharacterId, CharacterTag);
		CharacterTagToIds.Emplace(CharacterTag, CharacterId);
	}
}
