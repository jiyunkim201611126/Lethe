// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingWidget.h"

#include "DeckEditingCardListObject.h"
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

	if (UDeckManagerSubsystem* SaveManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>())
	{
		const TMap<FGameplayTag, FSavedCharacterDeck>& UnlockedCards = SaveManagerSubsystem->GetUnlockedCards();
		
		for (const auto& UnlockedCard : UnlockedCards)
		{
			for (const auto& Card : UnlockedCard.Value.Cards)
			{
				const ULetheGameplayAbility* CardAbilityCDO = Card.CardAbility.GetDefaultObject();
				CreateCard(CardAbilityCDO, UnlockedCard.Key);
			}
		}
	}
}

void UDeckEditingWidget::NativeDestruct()
{
	if (UnequippedCardTileView)
	{
		UnequippedCardTileView->OnItemClicked().RemoveAll(this);
	}
	
	Super::NativeDestruct();
}

void UDeckEditingWidget::CreateCard(const ULetheGameplayAbility* CardAbilityCDO, const FGameplayTag& InCharacterTag) const
{
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

	UnequippedCardTileView->AddItem(CardListObject);
}

void UDeckEditingWidget::OnItemClicked(UObject* InListObject) const
{
	UnequippedCardTileView->RemoveItem(InListObject);
}
