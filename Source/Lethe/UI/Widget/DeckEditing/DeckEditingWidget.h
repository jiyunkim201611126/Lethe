// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Blueprint/UserWidget.h"
#include "DeckEditingWidget.generated.h"

class UButton;
class UCardViewData;
class UDeckEditingCardListObject;
class ULetheGameplayAbility;
class UTileView;
struct FSavedCharacterDeck;
struct FGameplayTag;
struct FCardSelfViewInfo;
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
	void StartLoadAllCards();
	void StartLoadDecks(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, bool bEquipped);
	void OnAllCardsLoaded(const FGameplayTag& CharacterTag, const TArray<struct FLoadedCardInfo>& LoadedCards, bool bEquipped);

	UFUNCTION()
	void OnNextPageButtonClicked();
	
	UFUNCTION()
	void OnPreviousPageButtonClicked();
	
	UFUNCTION()
	void OnNextCharacterButtonClicked();
	
	UFUNCTION()
	void OnPreviousCharacterButtonClicked();

	void UpdateCardPage(const int32 NewCharacterIndex, const int32 NewPageIndex);

	void OnItemClicked(UObject* InListObject);

	bool CanAddCardToEquippedDeck(const FDeckListObjects* EquippedDeckListObjects, UDeckEditingCardListObject* InDeckObject) const;

	UFUNCTION()
	void OnGoToBattleButtonClicked();

protected:
	/**
	 * DeckEditing의 경우엔 WidgetController가 굳이 필요 없습니다.
	 * WidgetController는 보통 Model의 값 변화에 Widget이 바로바로 대응해야 하는 경우 사용합니다.
	 * 하지만 DeckEditing은 초기 Deck 상태를 Load해서 한 번 보여주고, 편집이 끝나면 Save 로직을 진행하며 위젯은 더이상 필요하지 않게 됩니다.
	 * 따라서 Model(Deck)의 상태 변화를 감시하고 있을 이유가 없으므로 위젯이 직접 Deck 정보를 가진 Subsystem에 접근하고, ViewData 또한 갖고 있는 방식으로 구현합니다.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;

	// 직선 배치지만, ScaleBox 활용을 위해 내부 요소의 Size를 직접 설정할 수 있는 TileView를 사용합니다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> EquippedDeckTileView;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTileView> UnequippedDeckTileView;
	
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
	// TODO: 현재는 SaveGame을 통해 불러온 값을 기반으로 작동하지만, 나중엔 GameInstance에 캐싱한 '전투 참여 캐릭터'를 받아와 사용할 예정입니다.
	TArray<FGameplayTag> CharacterTags;
	int32 CurrentCharacterIndex = 0;
	int32 CurrentPageIndex = 0;

	// TODO: 해상도에 따라 달라지는 로직이 필요합니다.
	int32 MaxCardCountInOnePage = 14;

	// 카드 로드 콜백에서 페이지를 표시하기 위해 사용하는 변수들입니다.
	// 모든 Unequipped 카드 로드가 끝나면 첫 페이지를 표시하도록 조정합니다.
	int32 ShouldLoadCardCount = 0;
	int32 LoadedCardCount = 0;

	// Key는 CharacterTag입니다.
	UPROPERTY()
	TMap<FGameplayTag, FDeckListObjects> CharacterUnequippedDeckListObjects;

	UPROPERTY()
	TMap<FGameplayTag, FDeckListObjects> CharacterEquippedDeckListObjects;
};
