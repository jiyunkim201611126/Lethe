// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "DeckEditingCardListObject.h"
#include "Components/Button.h"
#include "Components/TileView.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardViewData.h"
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

	if (UDeckManagerSubsystem* SaveManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>())
	{
		const TMap<FGameplayTag, FSavedCharacterDeck>& UnlockedCards = SaveManagerSubsystem->GetUnlockedCards();

		// 모든 카드를 순회하며 ListView에 들어갈 Object를 미리 만들어둡니다.
		for (const auto& UnlockedCard : UnlockedCards)
		{
			CharacterTags.Emplace(UnlockedCard.Key);
			
			for (const auto& Card : UnlockedCard.Value.Cards)
			{
				const ULetheGameplayAbility* CardAbilityCDO = Card.CardAbility.GetDefaultObject();
				CreateCardObject(CardAbilityCDO, UnlockedCard.Key);
			}
		}

		// 첫 캐릭터의 미장비 카드를 첫 페이지부터 표시합니다.
		UpdateCardPage(0, 0);
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

void UDeckEditingWidget::CreateCardObject(const ULetheGameplayAbility* CardAbilityCDO, const FGameplayTag& InCharacterTag)
{
	// ListView에 추가할 Object를 생성 후 캐싱해놓습니다.
	UDeckEditingCardListObject* CardListObject = NewObject<UDeckEditingCardListObject>();
	CardListObject->CardTag = CardAbilityCDO->CardTag;
	CardListObject->CharacterTag = InCharacterTag;
	
	FCardSelfViewInfo* CardSelfViewInfo = CardViewData->FindCardSelfViewInfoByTag(CardAbilityCDO->CardTag);
	if (CardSelfViewInfo->CardDescriptionText.IsEmpty())
	{
		const FText CardDescriptionText = CardAbilityCDO->GetCardDescription(CardAbilityCDO->GetAbilityLevel());
		CardSelfViewInfo->CardDescriptionText = CardDescriptionText;
	}
	CardListObject->CardSelfViewInfo = CardSelfViewInfo;

	CardListObject->CardTypeColor = CardViewData->FindCardTypeColor(CardAbilityCDO->CardTypeTag);

	FDeckListObjects& CardListObjects = CharacterUnequippedCardListObjects.FindOrAdd(InCharacterTag);
	CardListObjects.CardListObjects.Emplace(CardListObject);
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
