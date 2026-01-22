// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "UObject/Object.h"
#include "CardDataLoader.generated.h"

class UCardDefinitionData;
class UCardSelfViewData;
class UCardOwnerViewData;
class ULetheGameplayAbility;

DECLARE_DELEGATE_FourParams(OnLoadFinishedSignature, const ULetheGameplayAbility*, const UCardDefinitionData*, UCardSelfViewData*, const UCardOwnerViewData*)

/**
 * 카드 View 관련 데이터 로드 담당 객체입니다.
 * ASC가 AssetManager를 직접 참조해 사용하려니 함수가 길어지고 람다도 많아져서 선언되었습니다.
 */
UCLASS()
class LETHE_API UCardDataLoader : public UObject
{
	GENERATED_BODY()

public:
	void Init(const FGameplayTag& InCharacterTag, const UCardDefinitionData* InCardDefinition, const ULetheGameplayAbility* InAbility);

private:
	void LoadCardSelfViewData();
	void LoadCardOwnerViewData();

	void OnCardSelfViewDataLoaded(FPrimaryAssetId AssetId);
	void OnCardOwnerViewDataLoaded(FPrimaryAssetId AssetId);

	void TryFinish();

public:
	OnLoadFinishedSignature OnLoadFinishedDelegate;

private:
	FGameplayTag CharacterTag;

	UPROPERTY()
	TObjectPtr<const ULetheGameplayAbility> Ability;

	UPROPERTY()
	TObjectPtr<const UCardDefinitionData> CardDefinition;

	UPROPERTY()
	TObjectPtr<UCardSelfViewData> CardSelfViewData;

	UPROPERTY()
	TObjectPtr<UCardOwnerViewData> CardOwnerViewData;
};
