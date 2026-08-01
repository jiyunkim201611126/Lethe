// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "CardWidgetInitContext.h"
#include "CommonButtonBase.h"
#include "Components/TileView.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/Manager/CardDataLoadSubsystem.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/World/LevelManagerSubsystem.h"
#include "Lethe/UI/Core/LetheTextBlock.h"

void FDeckListObjects::Sort()
{
	CardListObjects.Sort([](const UCardWidgetInitContext& A, const UCardWidgetInitContext& B)
	{
		return A.SavedCard.CardId < B.SavedCard.CardId;
	});
}

int32 FDeckListObjects::GetEqualCardCount(const FGameplayTag& CardTag) const
{
	int32 EqualCardCount = 0;
	for (const UCardWidgetInitContext* CardListObject : CardListObjects)
	{
		if (CardListObject->SavedCard.CardTag == CardTag)
		{
			++EqualCardCount;
		}
	}
	return EqualCardCount;
}

int32 FDeckListObjects::GetTotalCardWeight() const
{
	int32 TotalWeight = 0;
	for (const UCardWidgetInitContext* ListObject : CardListObjects)
	{
		TotalWeight += ListObject->Weight;
	}
	return TotalWeight;
}

void UDeckEditingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	LoadRequestCount = 0;
	LoadCompletedCount = 0;

	EquippedCardClickedDelegateHandle = EquippedDeckTileView->OnItemClicked().AddUObject(this, &ThisClass::OnEquippedCardClicked);
	UnequippedCardClickedDelegateHandle = UnequippedDeckTileView->OnItemClicked().AddUObject(this, &ThisClass::OnUnequippedCardClicked);
	
	NextPageButton->OnClicked().AddUObject(this, &ThisClass::OnNextPageButtonClicked);
	PreviousPageButton->OnClicked().AddUObject(this, &ThisClass::OnPreviousPageButtonClicked);
	NextCharacterButton->OnClicked().AddUObject(this, &ThisClass::OnNextCharacterButtonClicked);
	PreviousCharacterButton->OnClicked().AddUObject(this, &ThisClass::OnPreviousCharacterButtonClicked);

	StartLoadAllCards();
}

void UDeckEditingWidget::NativeDestruct()
{
	EquippedDeckTileView->OnItemClicked().Remove(EquippedCardClickedDelegateHandle);
	UnequippedDeckTileView->OnItemClicked().Remove(UnequippedCardClickedDelegateHandle);

	GoToBattleButton->OnClicked().RemoveAll(this);
	NextPageButton->OnClicked().RemoveAll(this);
	PreviousPageButton->OnClicked().RemoveAll(this);
	NextCharacterButton->OnClicked().RemoveAll(this);
	PreviousCharacterButton->OnClicked().RemoveAll(this);

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
			const FOnAllCardDataLoaded OnLoadedCallback = FOnAllCardDataLoaded::CreateUObject(this, &ThisClass::OnDeckLoaded);
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

void UDeckEditingWidget::OnDeckLoaded(const FGameplayTag& CharacterTag, const FLoadedCardInfo& LoadedCardInfos, const bool bEquipped)
{
	++LoadCompletedCount;
	
	// 장착 상태에 따라 알맞은 위치에 추가될 수 있도록 TMap을 선택합니다.
	TMap<FGameplayTag, FDeckListObjects>& TargetDeckListObjects = bEquipped ? CharacterEquippedDeckListObjects : CharacterUnequippedDeckListObjects;
	FDeckListObjects* DeckListObjects = TargetDeckListObjects.Find(CharacterTag);

	if (DeckListObjects)
	{
		const UCharacterDefinitionData* CharacterDefinitionData = LoadedCardInfos.CharacterDefinition;
		if (!CharacterDefinitionData)
		{
			return;
		}

		if (!CharacterDeckCapacities.Contains(CharacterTag))
		{
			int32& DeckCapacity = CharacterDeckCapacities.Add(CharacterTag);
			DeckCapacity += CharacterDefinitionData->GetDeckCapacity(1);
		}
		
		for (const FCardInfo& CardInfo : LoadedCardInfos.LoadedCards)
		{
			if (!CardInfo.CardDefinition)
			{
				continue;
			}
		
			// TileView에 추가할 UObject 객체를 생성합니다.
			if (UCardWidgetInitContext* CardListObject = NewObject<UCardWidgetInitContext>(this))
			{
				// DeckEditingCardWidget의 초기화에 필요한 정보를 할당합니다.
				CardListObject->SavedCard = CardInfo.SavedCard;
				CardListObject->CardTypeColor = CardViewData->GetCardTypeColor(CardInfo.CardDefinition->CardTypeTag);
				CardListObject->CardTexture = CardInfo.CardDefinition->CardTexture;
				CardListObject->Weight = CardInfo.CardDefinition->GetWeight(CardInfo.SavedCard.CardLevel);

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
		if (!GoToBattleButton->OnClicked().IsBound())
		{
			GoToBattleButton->OnClicked().AddUObject(this, &ThisClass::OnGoToBattleButtonClicked);
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

void UDeckEditingWidget::OnEquippedCardClicked(UObject* InListObject)
{
	if (CharacterTags.IsValidIndex(CurrentCharacterIndex))
	{
		if (UCardWidgetInitContext* DeckListObject = Cast<UCardWidgetInitContext>(InListObject))
		{
			const FGameplayTag& CharacterTag = CharacterTags[CurrentCharacterIndex];

			FDeckListObjects* UnequippedDeckListObjects = CharacterUnequippedDeckListObjects.Find(CharacterTag);
			FDeckListObjects* EquippedDeckListObjects = CharacterEquippedDeckListObjects.Find(CharacterTag);

			if (UnequippedDeckListObjects && EquippedDeckListObjects)
			{
				EquippedDeckListObjects->CardListObjects.Remove(DeckListObject);
				UnequippedDeckListObjects->CardListObjects.Emplace(DeckListObject);
			}
			UpdateCardPage(CurrentCharacterIndex, CurrentPageIndex);
		}
	}
}

void UDeckEditingWidget::OnUnequippedCardClicked(UObject* InListObject)
{
	if (CharacterTags.IsValidIndex(CurrentCharacterIndex))
	{
		if (UCardWidgetInitContext* DeckListObject = Cast<UCardWidgetInitContext>(InListObject))
		{
			// 현재 표시되고 있는 캐릭터에 해당하는 CardListObjects들을 가져옵니다.
			const FGameplayTag& CharacterTag = CharacterTags[CurrentCharacterIndex];

			FDeckListObjects* UnequippedDeckListObjects = CharacterUnequippedDeckListObjects.Find(CharacterTag);
			FDeckListObjects* EquippedDeckListObjects = CharacterEquippedDeckListObjects.Find(CharacterTag);

			if (UnequippedDeckListObjects && EquippedDeckListObjects)
			{
				// 추가 가능한 상태인지 확인한 후 배열의 상태를 조정합니다.
				if (CanAddCardToEquippedDeck(EquippedDeckListObjects, CharacterTag, DeckListObject))
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

void UDeckEditingWidget::UpdateCardPage(const int32 NewCharacterIndex, const int32 NewPageIndex)
{
	// 인덱스가 유효한지 검사합니다. 0부터 CharacterTags.Num() - 1까지 순회합니다.
	CurrentCharacterIndex = FMath::WrapExclusive(NewCharacterIndex, 0, CharacterTags.Num());
	
	const FGameplayTag& CurrentCharacterTag = CharacterTags[CurrentCharacterIndex];
	if (!CharacterUnequippedDeckListObjects.Contains(CurrentCharacterTag))
	{
		return;
	}

	// 인덱스에 해당하는 캐릭터의 UnequippedListObjects를 가져옵니다.
	if (FDeckListObjects* UnequippedDeckListObjects = CharacterUnequippedDeckListObjects.Find(CurrentCharacterTag))
	{
		UnequippedDeckListObjects->Sort();
		const TArray<TObjectPtr<UCardWidgetInitContext>>& UnequippedDeckObjects = UnequippedDeckListObjects->CardListObjects;
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
	if (FDeckListObjects* EquippedDeckListObjects = CharacterEquippedDeckListObjects.Find(CurrentCharacterTag))
	{
		EquippedDeckListObjects->Sort();
		const TArray<TObjectPtr<UCardWidgetInitContext>>& EquippedDeckObjects = EquippedDeckListObjects->CardListObjects;

		// 장착 덱 페이지 갱신을 시작합니다.
		EquippedDeckTileView->ClearListItems();
		for (UCardWidgetInitContext* EquippedDeckObject : EquippedDeckObjects)
		{
			EquippedDeckTileView->AddItem(EquippedDeckObject);
		}

		const int32 CurrentTotalCardWeight = EquippedDeckListObjects->GetTotalCardWeight();
		const int32 DeckCapacity = CharacterDeckCapacities.FindRef(CurrentCharacterTag);

		CapacityTextBlock->SetText(FText::Format(FText::FromString(TEXT("{0} / {1}")), CurrentTotalCardWeight, DeckCapacity));
	}
}

bool UDeckEditingWidget::CanAddCardToEquippedDeck(const FDeckListObjects* EquippedDeckListObjects, const FGameplayTag& CharacterTag, const UCardWidgetInitContext* InitContext) const
{
	if (!EquippedDeckListObjects || !CharacterTag.IsValid()|| !InitContext)
	{
		return false;
	}
	
	if (EquippedDeckListObjects->CardListObjects.Num() >= MAX_DECK_COUNT)
	{
		// 장착 카드 개수가 10장을 초과할 수 없습니다.
		return false;
	}

	const int32 EqualCardCount = EquippedDeckListObjects->GetEqualCardCount(InitContext->SavedCard.CardTag);
	if (MaxEqualCardCount <= EqualCardCount)
	{
		// 동일한 카드를 3장 초과 장착할 수 없습니다.
		return false;
	}

	const int32 TotalCardWeight = EquippedDeckListObjects->GetTotalCardWeight() + InitContext->Weight;
	const int32 DeckCapacity = CharacterDeckCapacities.FindRef(CharacterTag);
	if (DeckCapacity <= 0 || DeckCapacity < TotalCardWeight)
	{
		// 카드 총합 무게가 캐릭터의 용량을 초과할 수 없습니다.
		return false;
	}

	// 위 조건에 모두 해당하지 않는다면 장착 가능한 상태라고 판단합니다.
	return true;
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
				SavedCharacterDeck.Deck.Emplace(CardListObject->SavedCard);
			}

			EquippedDecks.Emplace(EquippedDeckListObjects.Key, SavedCharacterDeck);
		}

		for (const auto& UnequippedDeckListObjects : CharacterUnequippedDeckListObjects)
		{
			FSavedCharacterDeck SavedCharacterDeck;
			for (const auto& CardListObject : UnequippedDeckListObjects.Value.CardListObjects)
			{
				SavedCharacterDeck.Deck.Emplace(CardListObject->SavedCard);
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
