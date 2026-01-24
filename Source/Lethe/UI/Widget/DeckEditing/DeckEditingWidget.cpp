// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "DeckEditingCardListObject.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Manager/DataLoadManagerSubsystem.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/World/LevelManagerSubsystem.h"

void UDeckEditingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ShouldLoadCardCount = 0;
	
	if (UnequippedDeckTileView)
	{
		UnequippedDeckTileView->OnItemClicked().AddUObject(this, &ThisClass::OnItemClicked);
	}
	
	NextPageButton->OnClicked.AddDynamic(this, &ThisClass::OnNextPageButtonClicked);
	PreviousPageButton->OnClicked.AddDynamic(this, &ThisClass::OnPreviousPageButtonClicked);
	NextCharacterButton->OnClicked.AddDynamic(this, &ThisClass::OnNextCharacterButtonClicked);
	PreviousCharacterButton->OnClicked.AddDynamic(this, &ThisClass::OnPreviousCharacterButtonClicked);

	GoToBattleButton->OnClicked.AddDynamic(this, &ThisClass::OnGoToBattleButtonClicked);

	StartLoadAllCards();
}

void UDeckEditingWidget::NativeDestruct()
{
	if (UnequippedDeckTileView)
	{
		UnequippedDeckTileView->OnItemClicked().RemoveAll(this);
	}

	NextPageButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNextPageButtonClicked);
	PreviousPageButton->OnClicked.RemoveDynamic(this, &ThisClass::OnPreviousPageButtonClicked);
	NextCharacterButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNextCharacterButtonClicked);
	PreviousCharacterButton->OnClicked.RemoveDynamic(this, &ThisClass::OnPreviousCharacterButtonClicked);

	CharacterTags.Empty();
	CharacterUnequippedDeckListObjects.Empty();
	
	Super::NativeDestruct();
}

void UDeckEditingWidget::StartLoadAllCards()
{
	UDeckManagerSubsystem* DeckManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>();
	if (DeckManagerSubsystem)
	{
		// 로드되어 있는 Deck들을 가져온 뒤, 필요한 카드 관련 에셋 로드를 시작합니다.
		const TMap<FGameplayTag, FSavedCharacterDeck>& UnequippedDecks = DeckManagerSubsystem->GetUnequippedDecks();
		StartLoadDecks(UnequippedDecks);

		const TMap<FGameplayTag, FSavedCharacterDeck>& EquippedDecks = DeckManagerSubsystem->GetEquippedDecks();
		StartLoadDecks(EquippedDecks);
	}
}

void UDeckEditingWidget::StartLoadDecks(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks)
{
	if (UDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>())
	{
		for (const auto& Deck : InDecks)
		{
			FGameplayTag CharacterTag = Deck.Key;
			
			// Emplace를 사용하면 이미 존재하는 값을 덮어씌우므로 아래와 같은 함수들을 사용합니다.
			CharacterTags.AddUnique(CharacterTag);
			CharacterEquippedDeckListObjects.FindOrAdd(CharacterTag);
			CharacterUnequippedDeckListObjects.FindOrAdd(CharacterTag);

			TArray<FGameplayTag> CardTags;
			for (const FSavedCard& SavedCard : Deck.Value.Deck)
			{
				CardTags.Emplace(SavedCard.CardTag);
			}

			ShouldLoadCardCount += Deck.Value.Deck.Num();

			const FOnCardDefinitionsLoaded OnCardDefinitionsLoaded = FOnCardDefinitionsLoaded::CreateWeakLambda(this, [this, CharacterTag](const TArray<UCardDefinitionData*>& CardDefinitionDatas)
			{
				OnCardDefinitionDataLoadFinished(CharacterTag, CardDefinitionDatas, false);
			});
			
			CardDataLoadManagerSubsystem->LoadCardDefinitionData(CardTags, OnCardDefinitionsLoaded);
		}
	}
}

void UDeckEditingWidget::OnCardDefinitionDataLoadFinished(const FGameplayTag& InCharacterTag, const TArray<UCardDefinitionData*>& CardDefinitionDatas, const bool bEquipped)
{
	if (UDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDataLoadManagerSubsystem>())
	{
		for (UCardDefinitionData* CardDefinitionData : CardDefinitionDatas)
		{
			const FOnCardViewLoaded OnLoadComplete = FOnCardViewLoaded::CreateWeakLambda(this, [this, CardDefinitionData, bEquipped](const UCardSelfViewData* SelfViewData, const UCharacterDefinitionData* CharacterDefinitionData)
			{
				OnCardViewDataLoadFinished(CardDefinitionData, SelfViewData, CharacterDefinitionData, bEquipped);
			});
			
			CardDataLoadManagerSubsystem->LoadCardViewData(CardDefinitionData->CardTag, InCharacterTag, OnLoadComplete);
		}
	}
}

void UDeckEditingWidget::OnCardViewDataLoadFinished(const UCardDefinitionData* CardDefinitionData, const UCardSelfViewData* CardSelfViewData, const UCharacterDefinitionData* CharacterDefinitionData, const bool bEquipped)
{
	// 로드가 완료되면 카드 위젯을 생성하는 데에 필요한 Object를 생성해 캐싱해둡니다.
	if (UDeckEditingCardListObject* CardListObject = NewObject<UDeckEditingCardListObject>())
	{
		// 나중에 저장할 때 편리하도록 구조체로 만들어서 들고 있도록 합니다.
		FSavedCard CardInfo;
		CardInfo.CardId = CardDefinitionData->CardId;
		CardInfo.CardTag = CardDefinitionData->CardTag;
		CardInfo.CardLevel = 1;
		CardListObject->CardInfo = CardInfo;
		CardListObject->CardTypeColor = CardViewData->FindCardTypeColor(CardDefinitionData->CardTypeTag);
		CardListObject->CardTexture = CardSelfViewData->CardTexture;

		TMap<FGameplayTag, FDeckListObjects>& SelectedDecks = bEquipped ? CharacterEquippedDeckListObjects : CharacterUnequippedDeckListObjects;
		if (FDeckListObjects* DeckListObjects = SelectedDecks.Find(CharacterDefinitionData->CharacterTag))
		{
			DeckListObjects->CardListObjects.Emplace(CardListObject);

			// 모든 Unequipped 카드 로드가 끝나면 첫 캐릭터의 첫 페이지를 표시하는 로직을 수행합니다.
			LoadedCardCount++;
			if (LoadedCardCount == ShouldLoadCardCount)
			{
				UpdateCardPage(0, 0);
			}
		}
	}
}

void UDeckEditingWidget::OnNextPageButtonClicked()
{
	UpdateCardPage(CurrentCharacterIndex, CurrentPageIndex + 1);
}

void UDeckEditingWidget::OnPreviousPageButtonClicked()
{
	UpdateCardPage(CurrentCharacterIndex, CurrentPageIndex - 1);
}

void UDeckEditingWidget::OnNextCharacterButtonClicked()
{
	UpdateCardPage(CurrentCharacterIndex + 1, 0);
}

void UDeckEditingWidget::OnPreviousCharacterButtonClicked()
{
	UpdateCardPage(CurrentCharacterIndex - 1, 0);
}

void UDeckEditingWidget::UpdateCardPage(const int32 NewCharacterIndex, const int32 NewPageIndex)
{
	// 인덱스가 유효한지 검사합니다. 0부터 CharacterTags.Num() - 1까지 순환합니다.
	CurrentCharacterIndex = FMath::WrapExclusive(NewCharacterIndex, 0, CharacterTags.Num());
	
	const FGameplayTag& CurrentCharacterTag = CharacterTags[CurrentCharacterIndex];
	if (!CharacterUnequippedDeckListObjects.Contains(CurrentCharacterTag))
	{
		return;
	}

	// 인덱스에 해당하는 캐릭터의 UnequippedListObjects를 가져옵니다.
	if (const FDeckListObjects* UnequippedDeckListObjects = CharacterUnequippedDeckListObjects.Find(CurrentCharacterTag))
	{
		const TArray<TObjectPtr<UDeckEditingCardListObject>>& UnequippedDeckObjects = UnequippedDeckListObjects->CardListObjects;
		const int32 TotalCards = UnequippedDeckObjects.Num();

		// 최대 페이지 수를 계산합니다.
		const int32 MaxPage = FMath::Max(0, (TotalCards - 1) / MaxCardCountInOnePage);
		// 이번에 열람할 페이지를 계산합니다. 0부터 MaxPage까지 순환합니다.
		CurrentPageIndex = FMath::WrapExclusive(NewPageIndex, 0, MaxPage + 1);
		// 이번에 열람할 페이지의 시작 카드 인덱스를 계산합니다.
		const int32 StartDataIndex = CurrentPageIndex * MaxCardCountInOnePage;

		// 미장착 덱 페이지 갱신을 시작합니다.
		UnequippedDeckTileView->ClearListItems();

		const int32 EndIndex = FMath::Min(StartDataIndex + MaxCardCountInOnePage, TotalCards);
		for (int32 Index = StartDataIndex; Index < EndIndex; ++Index)
		{
			UnequippedDeckTileView->AddItem(UnequippedDeckObjects[Index]);
		}
	}

	// 인덱스에 해당하는 캐릭터의 EquippedListObjects를 가져옵니다.
	if (const FDeckListObjects* EquippedDeckListObjects = CharacterEquippedDeckListObjects.Find(CurrentCharacterTag))
	{
		const TArray<TObjectPtr<UDeckEditingCardListObject>>& EquippedDeckObjects = EquippedDeckListObjects->CardListObjects;

		// 장착 덱 페이지 갱신을 시작합니다.
		EquippedDeckTileView->ClearListItems();
		for (UDeckEditingCardListObject* EquippedDeckObject : EquippedDeckObjects)
		{
			EquippedDeckTileView->AddItem(EquippedDeckObject);
		}
	}
}

void UDeckEditingWidget::OnItemClicked(UObject* InListObject)
{
	if (CharacterTags.IsValidIndex(CurrentCharacterIndex))
	{
		if (UDeckEditingCardListObject* DeckListObject = Cast<UDeckEditingCardListObject>(InListObject))
		{
			// 현재 표시되고 있는 캐릭터에 해당하는 CardListObjects들을 가져옵니다.
			const FGameplayTag& CharacterTag = CharacterTags[CurrentCharacterIndex];

			FDeckListObjects* UnequippedDeckListObjects = CharacterUnequippedDeckListObjects.Find(CharacterTag);
			FDeckListObjects* EquippedDeckListObjects = CharacterEquippedDeckListObjects.Find(CharacterTag);

			if (UnequippedDeckListObjects && EquippedDeckListObjects)
			{
				// 추가 가능한 상태인지 확인한 후 배열의 상태를 조정합니다.
				if (CanAddCardToEquippedDeck(EquippedDeckListObjects, DeckListObject))
				{
					UnequippedDeckListObjects->CardListObjects.Remove(DeckListObject);
					EquippedDeckListObjects->CardListObjects.Emplace(DeckListObject);
				}
			}
		
			// 페이지를 새로고침합니다.
			UpdateCardPage(CurrentCharacterIndex, CurrentPageIndex);
		}
	}
}

bool UDeckEditingWidget::CanAddCardToEquippedDeck(const FDeckListObjects* EquippedDeckListObjects, UDeckEditingCardListObject* InDeckObject) const
{
	if (const UDeckEditingCardListObject* DeckObject = Cast<UDeckEditingCardListObject>(InDeckObject))
	{
		if (EquippedDeckListObjects->CardListObjects.Num() >= MAX_DECK_COUNT)
		{
			// 장착 카드 개수가 10장을 초과할 수 없습니다.
			return false;
		}

		const FGameplayTag& CardTag = DeckObject->CardInfo.CardTag;
		constexpr int32 MaxEqualCardCount = 3;
		int32 EqualCardCount = 0;
		for (const UDeckEditingCardListObject* EquippedDeckListObject : EquippedDeckListObjects->CardListObjects)
		{
			if (EquippedDeckListObject->CardInfo.CardTag == CardTag)
			{
				++EqualCardCount;
			}
		}

		if (EqualCardCount >= MaxEqualCardCount)
		{
			// 동일한 카드를 3장 초과 장착할 수 없습니다.
			return false;
		}

		// 위 조건에 모두 해당하지 않는다면 장착 가능한 상태라고 판단합니다.
		return true;
	}

	return false;
}

void UDeckEditingWidget::OnGoToBattleButtonClicked()
{
	if (UDeckManagerSubsystem* DeckManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>())
	{
		// 위젯용 UObject를 DeckManagerSubsystem에서 사용하는 구조체로 가공합니다.
		TMap<FGameplayTag, FSavedCharacterDeck> EquippedDecks;
		TMap<FGameplayTag, FSavedCharacterDeck> UnequippedDecks;

		for (const auto& EquippedDeckListObjects : CharacterEquippedDeckListObjects)
		{
			FSavedCharacterDeck SavedCharacterDeck;
			for (const auto& CardListObject : EquippedDeckListObjects.Value.CardListObjects)
			{
				SavedCharacterDeck.Deck.Emplace(CardListObject->CardInfo);
			}

			EquippedDecks.Emplace(EquippedDeckListObjects.Key, SavedCharacterDeck);
		}

		for (const auto& UnequippedDeckListObjects : CharacterUnequippedDeckListObjects)
		{
			FSavedCharacterDeck SavedCharacterDeck;
			for (const auto& CardListObject : UnequippedDeckListObjects.Value.CardListObjects)
			{
				SavedCharacterDeck.Deck.Emplace(CardListObject->CardInfo);
			}

			UnequippedDecks.Emplace(UnequippedDeckListObjects.Key, SavedCharacterDeck);
		}

		// 미장착 / 장착 상태의 덱들을 저장합니다.
		DeckManagerSubsystem->SaveDeck(EquippedDecks, UnequippedDecks);
	}

	// 레벨 이동을 시작합니다.
	if (ULevelManagerSubsystem* LevelManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>())
	{
		LevelManagerSubsystem->ChangeMap(ELevelType::Battle, "FromDeckEditing");
	}
}
