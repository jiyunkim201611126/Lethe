// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "GameplayTagContainer.h"
#include "LetheAssetManager.generated.h"

class UPrimaryDataAsset;

DECLARE_DELEGATE_OneParam(FOnPrimaryDataAssetsLoaded, const TArray<UPrimaryDataAsset*>&)

UCLASS()
class LETHE_API ULetheAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	static ULetheAssetManager& Get();
	
	/** 에셋 색인 후 관련 정보를 사용하기 편리하도록 캐싱하는 함수입니다. */
	void BuildAssetIdCaches();

	/** GameplayTag 배열에 매핑된 PrimaryDataAsset 로드를 요청합니다. */
	void LoadPrimaryDataAssets(const TArray<FGameplayTag>& InGameplayTags, FOnPrimaryDataAssetsLoaded OnComplete);

	bool TryGetAssetId(const FGameplayTag& GameplayTag, FPrimaryAssetId& OutAssetId) const;

	bool TryGetCardTagById(uint64 CardId, FGameplayTag& OutCardTag) const;
	bool TryGetCardIdByTag(const FGameplayTag& CardTag, uint64& OutCardId) const;

	bool TryGetCharacterTagById(uint64 CharacterId, FGameplayTag& OutCharacterTag) const;
	bool TryGetCharacterIdByTag(const FGameplayTag& CharacterTag, uint64& OutCharacterId) const;

protected:
	//~ Begin UAssetManager Interface
	virtual void StartInitialLoading() override;
	//~ End of UAssetManager Interface

private:
	void OnPrimaryDataAssetsLoaded(const TArray<FPrimaryAssetId>& LoadedAssetsId, const FOnPrimaryDataAssetsLoaded& OnComplete) const;

	void BuildCardDefinitionCache();
	void BuildCharacterDefinitionCache();

private:
	TMap<FGameplayTag, FPrimaryAssetId> TagToAssetId;

	TMap<uint64, FGameplayTag> CardIdToTag;
	TMap<FGameplayTag, uint64> CardTagToId;

	TMap<uint64, FGameplayTag> CharacterIdToTag;
	TMap<FGameplayTag, uint64> CharacterTagToId;
};
