// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"
#include "CardPrimaryDataAssetLoader.generated.h"

class UCardDefinitionData;
class UCardSelfViewData;
class UCharacterDefinitionData;
class UDataLoadManagerSubsystem;
struct FSavedCard;

USTRUCT(BlueprintType)
struct FLoadedCardInfo
{
	GENERATED_BODY()

	UPROPERTY()
	UCardDefinitionData* CardDefinition;

	UPROPERTY()
	UCardSelfViewData* SelfViewData;

	UPROPERTY()
	UCharacterDefinitionData* CharacterDefinition;
};

DECLARE_DELEGATE_ThreeParams(FOnAllCardDataLoaded, const FGameplayTag& /*CharacterTag*/, const TArray<FLoadedCardInfo>& /*LoadedCards*/, const bool /*bEquipped*/);

/**
 * DataLoadManagerSubsystem를 참조해 DataAsset 로드를 요청하는 담당 클래스입니다.
 * 코드 중복을 제거하기 위해 선언되었습니다.
 * 
 * 이 클래스를 사용하는 곳에선 콜백 함수를 1개만 선언해 사용할 수 있습니다.
 * 단, 이 클래스를 사용해 에셋을 로드하려는 경우 CardDefinition, CardSelfView, CharacterDefinition을 모두 콜백으로 한 번에 받아야 합니다.
 */
UCLASS()
class LETHE_API UCardPrimaryDataAssetLoader : public UObject
{
	GENERATED_BODY()

public:
	static UCardPrimaryDataAssetLoader* CreateLoader(UObject* Outer);

	void LoadCardData(const FGameplayTag& CharacterTag, const TArray<FSavedCard>& Cards, bool bEquipped, const FOnAllCardDataLoaded& OnLoadedCallback);

private:
	void OnCardDefinitionsLoaded(const TArray<UCardDefinitionData*>& LoadedDefinitions);
	void OnViewDataLoaded(UCardDefinitionData* CardDefinition, UCardSelfViewData* SelfView, UCharacterDefinitionData* CharacterDefinition);
	void CheckLoadFinished();

	void SelfDestruct();

	UPROPERTY()
	TObjectPtr<UDataLoadManagerSubsystem> DataLoadManager;
	
	FGameplayTag ForCharacterTag;
	bool bIsFromEquippedDeck;
	FOnAllCardDataLoaded OnAllDataLoaded;

	UPROPERTY()
	TArray<FLoadedCardInfo> LoadedCardInfos;
	
	int32 TotalShouldLoadCount = 0;
	int32 CurrentLoadedCount = 0;
};
