// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GameplayTagContainer.h"
#include "LetheAssetManager.generated.h"

UCLASS()
class LETHE_API ULetheAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static ULetheAssetManager& Get();
	
	/** 에셋 색인 후 관련 정보를 사용하기 편리하도록 캐싱하는 함수입니다. */
	void BuildAssetIdCaches();

	bool TryGetCardDefinitionAssetId(const FGameplayTag& CardTag, FPrimaryAssetId& OutAssetId) const;
	bool TryGetCardSelfViewAssetId(const FGameplayTag& CardTag, FPrimaryAssetId& OutAssetId) const;
	bool TryGetCharacterDefinitionAssetId(const FGameplayTag& CharacterTag, FPrimaryAssetId& OutAssetId) const;

	bool TryGetCardTagById(uint64 CardId, FGameplayTag& OutCardTag) const;
	bool TryGetCardIdByTag(const FGameplayTag& CardTag, uint64& OutCardId) const;

	bool TryGetCharacterTagById(uint64 CharacterId, FGameplayTag& OutCharacterTag) const;
	bool TryGetCharacterIdByTag(const FGameplayTag& CharacterTag, uint64& OutCharacterId) const;

protected:
	//~ Begin UAssetManager Interface
	virtual void StartInitialLoading() override;
	//~ End of UAssetManager Interface

private:
	void BuildCardDefinitionCache();
	void BuildCardSelfViewCache();
	void BuildCharacterDefinitionCache();

private:
	TMap<FGameplayTag, FPrimaryAssetId> CardDefinitionAssetIds;
	TMap<FGameplayTag, FPrimaryAssetId> CardSelfViewAssetIds;
	TMap<FGameplayTag, FPrimaryAssetId> CharacterDefinitionAssetIds;

	TMap<uint64, FGameplayTag> CardIdToTags;
	TMap<FGameplayTag, uint64> CardTagToIds;

	TMap<uint64, FGameplayTag> CharacterIdToTags;
	TMap<FGameplayTag, uint64> CharacterTagToIds;
};
