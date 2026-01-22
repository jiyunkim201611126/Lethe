// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "DeckEditingCardListObject.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardDefinitionData.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/Manager/CardDataLoadManagerSubsystem.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"

void UDeckEditingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UnequippedCardTileView)
	{
		UnequippedCardTileView->OnItemClicked().AddUObject(this, &ThisClass::OnItemClicked);
	}
	
	NextPageButton->OnClicked.AddDynamic(this, &ThisClass::OnNextPageButtonClicked);
	PreviousPageButton->OnClicked.AddDynamic(this, &ThisClass::OnPreviousPageButtonClicked);
	NextCharacterButton->OnClicked.AddDynamic(this, &ThisClass::OnNextCharacterButtonClicked);
	PreviousCharacterButton->OnClicked.AddDynamic(this, &ThisClass::OnPreviousCharacterButtonClicked);

	UDeckManagerSubsystem* DeckManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>();
	UCardDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCardDataLoadManagerSubsystem>();
	if (DeckManagerSubsystem && CardDataLoadManagerSubsystem)
	{
		const TMap<FGameplayTag, FSavedCharacterDeck>& UnlockedCards = DeckManagerSubsystem->GetUnlockedCards();

		for (const auto& UnlockedCard : UnlockedCards)
		{
			FGameplayTag CharacterTag = UnlockedCard.Key;
			CharacterTags.Emplace(CharacterTag);

			TArray<FGameplayTag> CardTags;
			for (const FSavedCard& SavedCard : UnlockedCard.Value.Cards)
			{
				CardTags.Emplace(SavedCard.CardTag);
			}

			ShouldLoadCardCount += UnlockedCard.Value.Cards.Num();

			CardDataLoadManagerSubsystem->LoadCardDefinitionData(CardTags, FOnCardDefinitionsLoaded::CreateWeakLambda(this, [this, CharacterTag](const TArray<UCardDefinitionData*>& CardDefinitionDatas)
			{
				OnCardDefinitionDataLoadFinished(CharacterTag, CardDefinitionDatas);
			}));
		}
	}
}

void UDeckEditingWidget::NativeDestruct()
{
	if (UnequippedCardTileView)
	{
		UnequippedCardTileView->OnItemClicked().RemoveAll(this);
	}

	NextPageButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNextPageButtonClicked);
	PreviousPageButton->OnClicked.RemoveDynamic(this, &ThisClass::OnPreviousPageButtonClicked);
	NextCharacterButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNextCharacterButtonClicked);
	PreviousCharacterButton->OnClicked.RemoveDynamic(this, &ThisClass::OnPreviousCharacterButtonClicked);

	CharacterTags.Empty();
	CharacterUnequippedCardListObjects.Empty();
	
	Super::NativeDestruct();
}

void UDeckEditingWidget::OnCardDefinitionDataLoadFinished(const FGameplayTag& InCharacterTag, const TArray<UCardDefinitionData*>& CardDefinitionDatas)
{
	if (UCardDataLoadManagerSubsystem* CardDataLoadManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UCardDataLoadManagerSubsystem>())
	{
		for (UCardDefinitionData* CardDefinitionData : CardDefinitionDatas)
		{
			const FOnCardViewLoaded OnLoadComplete = FOnCardViewLoaded::CreateWeakLambda(this, [this, CardDefinitionData](UCardSelfViewData* SelfViewData, const UCardOwnerViewData* OwnerViewData)
			{
				OnCardViewDataLoadFinished(CardDefinitionData, SelfViewData, OwnerViewData);
			});
			
			CardDataLoadManagerSubsystem->LoadCardViewData(CardDefinitionData->CardTag, InCharacterTag, OnLoadComplete);
		}
	}
}

void UDeckEditingWidget::OnCardViewDataLoadFinished(const UCardDefinitionData* CardDefinitionData, UCardSelfViewData* CardSelfViewData, const UCardOwnerViewData* CardOwnerViewData)
{
	// Ability에서 CardDescription을 가져와 DataAsset에 넣어줍니다.
	if (CardDefinitionData && CardSelfViewData)
	{
		// 덱 편집 중이므로 설명은 Level 1 기준으로 넣어줍니다.
		const ULetheGameplayAbility* Ability = CardDefinitionData->AbilityClass->GetDefaultObject<ULetheGameplayAbility>();
		CardSelfViewData->CardDescriptionText = Ability->GetCardDescription(1);
	}

	// 로드가 완료되면 카드 위젯을 생성하는 데에 필요한 Object를 생성해 캐싱해둡니다.
	if (UDeckEditingCardListObject* CardListObject = NewObject<UDeckEditingCardListObject>())
	{
		CardListObject->CardTypeColor = CardViewData->FindCardTypeColor(CardDefinitionData->CardTypeTag);
		CardListObject->CardTexture = CardSelfViewData->CardTexture;

		FDeckListObjects& DeckListObjects = CharacterUnequippedCardListObjects.FindOrAdd(CardOwnerViewData->CharacterTag);
		DeckListObjects.CardListObjects.Emplace(CardListObject);

		LoadedCardCount++;

		// 모든 Unequipped 카드 로드가 끝나면 첫 캐릭터의 첫 페이지를 표시하는 로직을 수행합니다.
		if (LoadedCardCount == ShouldLoadCardCount)
		{
			UpdateCardPage(0, 0);
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
	if (!CharacterUnequippedCardListObjects.Contains(CurrentCharacterTag))
	{
		return;
	}

	// 인덱스에 해당하는 캐릭터의 덱 ListObjects를 가져옵니다.
	if (const FDeckListObjects* DeckListObjects = CharacterUnequippedCardListObjects.Find(CurrentCharacterTag))
	{
		const TArray<TObjectPtr<UDeckEditingCardListObject>>& CharacterDeckObjects = DeckListObjects->CardListObjects;
		const int32 TotalCards = CharacterDeckObjects.Num();

		// 최대 페이지 수를 계산합니다.
		const int32 MaxPage = FMath::Max(0, (TotalCards - 1) / MaxCardCountInOnePage);
		// 이번에 열람할 페이지를 계산합니다. 0부터 MaxPage까지 순환합니다.
		CurrentPageIndex = FMath::WrapExclusive(NewPageIndex, 0, MaxPage + 1);
		// 이번에 열람할 페이지의 시작 카드 인덱스를 계산합니다.
		const int32 StartDataIndex = CurrentPageIndex * MaxCardCountInOnePage;

		// 페이지 갱신을 시작합니다.
		UnequippedCardTileView->ClearListItems();

		const int32 EndIndex = FMath::Min(StartDataIndex + MaxCardCountInOnePage, TotalCards);
		for (int32 Index = StartDataIndex; Index < EndIndex; ++Index)
		{
			UnequippedCardTileView->AddItem(CharacterDeckObjects[Index]);
		}
	}
}

void UDeckEditingWidget::OnItemClicked(UObject* InListObject) const
{
	UnequippedCardTileView->RemoveItem(InListObject);
}
