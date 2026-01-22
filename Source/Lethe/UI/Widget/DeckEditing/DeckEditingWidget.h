// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardOwnerViewData.h"
#include "Lethe/Data/CardSelfViewData.h"
#include "DeckEditingWidget.generated.h"

struct FGameplayTag;
struct FCardSelfViewInfo;
struct FCardOwnerViewInfo;
class ULetheGameplayAbility;
class UCardViewData;
class UTileView;
class UButton;
class UDeckEditingCardListObject;
enum class ECardAction : uint8;

USTRUCT()
struct FDeckListObjects
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<TObjectPtr<UDeckEditingCardListObject>> CardListObjects;
};

UCLASS()
class LETHE_API UDeckEditingWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	//~ End of UUserWidget Interface

private:
	void StartLoadCardViewData(const FGameplayTag& InCharacterTag, const TArray<FPrimaryAssetId>& InPrimaryAssetIds);
	void OnCardViewDataLoadFinished(const ULetheGameplayAbility* Ability, const UCardDefinitionData* CardDefinitionData, UCardSelfViewData* CardSelfViewData, const UCardOwnerViewData* CardOwnerViewData) const;

	UFUNCTION()
	void OnNextPageButtonClicked();
	
	UFUNCTION()
	void OnPreviousPageButtonClicked();
	
	UFUNCTION()
	void OnNextCharacterButtonClicked();
	
	UFUNCTION()
	void OnPreviousCharacterButtonClicked();

	void UpdateCardPage(const int32 NewCharacterIndex, const int32 NewPageIndex);

	void OnItemClicked(UObject* InListObject) const;

protected:
	/**
	 * DeckEditing의 경우엔 WidgetController가 굳이 필요 없습니다.
	 * WidgetController는 보통 Model의 값 변화에 Widget이 바로바로 대응해야 하는 경우 사용합니다.
	 * 하지만 DeckEditing은 초기 Deck 상태를 Load해서 한 번 보여주고, 편집이 끝나면 Save 로직을 진행하며 위젯은 더이상 필요하지 않게 됩니다.
	 * 따라서 Model(Deck)의 상태 변화를 감시하고 있을 이유가 없으므로 위젯이 직접 Deck 정보를 가진 Subsystem에 접근하고, ViewData 또한 갖고 있는 방식으로 구현합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> EquippedCardTileView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> UnequippedCardTileView;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextPageButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PreviousPageButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> NextCharacterButton;
	
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> PreviousCharacterButton;

	// 테스트용으로 선언된 버튼입니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> GoToBattleButton;

private:
	// 덱들을 순서대로 순회할 CharacterTags입니다.
	// TODO: 지금은 직접 내부 요소를 채우지만, 추후에 현재 캐릭터 정보 등을 받아올 예정입니다.
	TArray<FGameplayTag> CharacterTags;
	int32 CurrentCharacterIndex = 0;
	int32 CurrentPageIndex = 0;

	int32 MaxCardCountInOnePage = 14;

	UPROPERTY()
	TMap<FGameplayTag, FDeckListObjects> CharacterUnequippedCardListObjects;
};
