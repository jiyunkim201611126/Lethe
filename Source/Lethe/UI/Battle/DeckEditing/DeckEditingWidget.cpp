// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "DeckEditingCardListObject.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/Manager/CardDataLoadSubsystem.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/World/LevelManagerSubsystem.h"

void UDeckEditingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadRequestCount = 0;
	LoadCompletedCount = 0;
	
	if (UnequippedDeckTileView)
	{
		UnequippedDeckTileView->OnItemClicked().AddUObject(this, &ThisClass::OnItemClicked);
	}
	
	NextPageButton->OnClicked.AddDynamic(this, &ThisClass::OnNextPageButtonClicked);
	PreviousPageButton->OnClicked.AddDynamic(this, &ThisClass::OnPreviousPageButtonClicked);
	NextCharacterButton->OnClicked.AddDynamic(this, &ThisClass::OnNextCharacterButtonClicked);
	PreviousCharacterButton->OnClicked.AddDynamic(this, &ThisClass::OnPreviousCharacterButtonClicked);

	StartLoadAllCards();
}

void UDeckEditingWidget::NativeDestruct()
{
	if (UnequippedDeckTileView)
	{
		UnequippedDeckTileView->OnItemClicked().RemoveAll(this);
	}

	GoToBattleButton->OnClicked.RemoveDynamic(this, &ThisClass::OnGoToBattleButtonClicked);
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
	if (UDeckManagerSubsystem* DeckManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>())
	{
		// 로드되어 있는 Deck들을 가져온 뒤, 필요한 카드 관련 에셋 로드를 시작합니다.
		const TMap<FGameplayTag, FSavedCharacterDeck>& UnequippedDecks = DeckManagerSubsystem->GetUnequippedDecks();
		const TMap<FGameplayTag, FSavedCharacterDeck>& EquippedDecks = DeckManagerSubsystem->GetEquippedDecks();
		LoadRequestCount += UnequippedDecks.Num();
		LoadRequestCount += EquippedDecks.Num();
		
		StartLoadDecks(UnequippedDecks, false);
		StartLoadDecks(EquippedDecks, true);
	}
}

void UDeckEditingWidget::StartLoadDecks(const TMap<FGameplayTag, FSavedCharacterDeck>& InDecks, const bool bEquipped)
{
	for (const auto& Deck : InDecks)
	{
		FGameplayTag CharacterTag = Deck.Key;

		CharacterTags.AddUnique(CharacterTag);
		CharacterEquippedDeckListObjects.FindOrAdd(CharacterTag);
		CharacterUnequippedDeckListObjects.FindOrAdd(CharacterTag);

		UCardDataLoadSubsystem* CardDataLoadSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCardDataLoadSubsystem>();
		if (CardDataLoadSubsystem)
		{
			const FOnAllCardDataLoaded OnLoadedCallback = FOnAllCardDataLoaded::CreateUObject(this, &ThisClass::OnAllCardsLoaded);
			CardDataLoadSubsystem->LoadCardData(CharacterTag, Deck.Value.Deck, bEquipped, OnLoadedCallback);
		}
		else
		{
			// 여기서도 카운트를 증가시켜줘야 최종적으로 RequestCount와 동일해져 로직적 문제가 발생하지 않습니다.
			++LoadCompletedCount;
			CheckLoadedCount();
		}
	}
}

void UDeckEditingWidget::OnAllCardsLoaded(const FGameplayTag& CharacterTag, const TArray<FLoadedCardInfo>& LoadedCards, const bool bEquipped)
{
	++LoadCompletedCount;
	
	// 장착 상태에 따라 알맞은 위치에 추가될 수 있도록 TMap을 선택합니다.
	TMap<FGameplayTag, FDeckListObjects>& TargetDeckListObjects = bEquipped ? CharacterEquippedDeckListObjects : CharacterUnequippedDeckListObjects;
	FDeckListObjects* DeckListObjects = TargetDeckListObjects.Find(CharacterTag);

	if (DeckListObjects)
	{
		for (const FLoadedCardInfo& CardInfo : LoadedCards)
		{
			if (!CardInfo.CardDefinition)
			{
				continue;
			}
		
			// TileView에 추가할 UObject 객체를 생성합니다.
			if (UDeckEditingCardListObject* CardListObject = NewObject<UDeckEditingCardListObject>(this))
			{
				// DeckEditingCardWidget의 초기화에 필요한 정보를 할당합니다.			
				CardListObject->SavedCardInfo = CardInfo.SavedCardInfo;
				CardListObject->CardTypeColor = CardViewData->FindCardTypeColor(CardInfo.CardDefinition->CardTypeTag);
				CardListObject->CardTexture = CardInfo.CardDefinition->CardTexture;

				// 일단 캐싱해둔 후 유저의 조작에 맞춰 TileView에 추가/제거하면서 업데이트합니다.
				DeckListObjects->CardListObjects.Emplace(CardListObject);
			}
		}
		CheckLoadedCount();
	}
}

void UDeckEditingWidget::CheckLoadedCount()
{
	if (LoadRequestCount <= LoadCompletedCount)
	{
		UpdateCardPage(0, 0);

		// 로드가 안 끝났는데 BattleLevel로 넘어가버리는 대참사를 막기 위해 여기서 바인드합니다.
		if (!GoToBattleButton->OnClicked.IsAlreadyBound(this, &ThisClass::OnGoToBattleButtonClicked))
		{
			GoToBattleButton->OnClicked.AddDynamic(this, &ThisClass::OnGoToBattleButtonClicked);
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

		const FGameplayTag& CardTag = DeckObject->SavedCardInfo.CardTag;
		constexpr int32 MaxEqualCardCount = 3;
		int32 EqualCardCount = 0;
		for (const UDeckEditingCardListObject* EquippedDeckListObject : EquippedDeckListObjects->CardListObjects)
		{
			if (EquippedDeckListObject->SavedCardInfo.CardTag == CardTag)
			{
				++EqualCardCount;
			}
		}

		if (MaxEqualCardCount <= EqualCardCount)
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
				SavedCharacterDeck.Deck.Emplace(CardListObject->SavedCardInfo);
			}

			EquippedDecks.Emplace(EquippedDeckListObjects.Key, SavedCharacterDeck);
		}

		for (const auto& UnequippedDeckListObjects : CharacterUnequippedDeckListObjects)
		{
			FSavedCharacterDeck SavedCharacterDeck;
			for (const auto& CardListObject : UnequippedDeckListObjects.Value.CardListObjects)
			{
				SavedCharacterDeck.Deck.Emplace(CardListObject->SavedCardInfo);
			}

			UnequippedDecks.Emplace(UnequippedDeckListObjects.Key, SavedCharacterDeck);
		}

		// 미장착 / 장착 상태의 덱들을 저장합니다.
		DeckManagerSubsystem->SaveDeck(EquippedDecks, UnequippedDecks);
	}

	// 레벨 이동을 시작합니다.
	if (ULevelManagerSubsystem* LevelManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULevelManagerSubsystem>())
	{
		LevelManagerSubsystem->StartLevelTransition(ELevelType::Battle, "FromDeckEditing");
	}
}
