// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardLayoutManager.h"

#include "Components/CanvasPanelSlot.h"
#include "Lethe/Lethe.h"
#include "Lethe/Util.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/UI/Framework/LetheWidgetController.h"
#include "Slate/WidgetTransform.h"

void UCardLayoutManager::Initialize(const FVector2D& CardSize)
{/*
	PaddingDeckAndHand += CardSize.X;
	PaddingHandAndHand += CardSize.X;
	FirstCardTranslation.X += CardSize.X / 2.f;
	FirstCardTranslation.Y -= CardSize.Y / 2.f;
	NextCardTranslation.X += CardSize.X / 2.f;
	NextCardTranslation.Y -= CardSize.Y / 2.f;
	GravesCardTranslation.X += CardSize.X / 2.f;
	GravesCardTranslation.Y -= CardSize.Y / 2.f;
	
	ASCToCards.Reserve(PLAYER_CHARACTER_NUMBER);*/
}

void UCardLayoutManager::SetupCardSlot(UCanvasPanelSlot* CardSlot) const
{
	if (CardSlot)
	{
		CardSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CardSlot->SetAutoSize(true);
		//CardSlot->SetZOrder(DeckZOrder);
	}
}
/*
void UCardLayoutManager::AddCardToDeck(UCardWidget* CardWidget)
{
	if (CardWidget)
	{
		if (ULetheAbilitySystemComponent* LetheASC = CardWidget->GetOwnerASC())
		{
			FCharacterCards& CharacterCards = ASCToCards.FindOrAdd(LetheASC);
			CharacterCards.Deck.Add(CardWidget);
		}
	}
}

void UCardLayoutManager::ShuffleDeck()
{
	for (auto& Cards : ASCToCards)
	{
		FRandomStream Stream(12345);
		ArrayShuffle::ShuffleWithSeed(Cards.Value.Deck, Stream);
	}
}

UCardWidget* UCardLayoutManager::GetTopDeckCard(ULetheAbilitySystemComponent* OwnerASC) const
{
	if (OwnerASC)
	{
		const FCharacterCards* CharacterCards = ASCToCards.Find(OwnerASC);
		if (CharacterCards && !CharacterCards->Deck.IsEmpty())
		{
			return CharacterCards->Deck.Last();
		}
	}
	return nullptr;
}

bool UCardLayoutManager::TryDraw(ULetheAbilitySystemComponent* OwnerASC)
{
	if (OwnerASC)
	{
		FCharacterCards* CharacterCards = ASCToCards.Find(OwnerASC);
		if (CharacterCards && !CharacterCards->Deck.IsEmpty())
		{
			if (UCardWidget* DrawnCard = CharacterCards->Deck.Pop(EAllowShrinking::No))
			{
				CharacterCards->Hands.Add(DrawnCard);
				DrawnCard->SetCardContainer(ECardContainer::Hand);
				return true;
			}
		}
	}
	return false;
}

void UCardLayoutManager::OnCardSelected(const UCardWidget* CardWidget) const
{
	if (CardWidget)
	{
		if (UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(CardWidget->Slot))
		{
			CardSlot->SetZOrder(SelectedZOrder);
		}
	}
}

void UCardLayoutManager::AddCardToGrave(UCardWidget* CardWidget)
{
	if (CardWidget && CardWidget->GetCurrentCardContainer() != ECardContainer::Grave)
	{
		if (ULetheAbilitySystemComponent* LetheASC = CardWidget->GetOwnerASC())
		{
			if (FCharacterCards* CharacterCards = ASCToCards.Find(LetheASC))
			{
				CharacterCards->Graves.AddUnique(CardWidget);
			}
			MoveCardToGrave(CardWidget);
			CardWidget->SetCardContainer(ECardContainer::Grave);
		}
	}
}

void UCardLayoutManager::AddAllHandsToGrave()
{
	for (auto& Cards : ASCToCards)
	{
		for (UCardWidget* Hand : Cards.Value.Hands)
		{
			AddCardToGrave(Hand);
		}
		Cards.Value.Hands.Reset();
	}
}

void UCardLayoutManager::RefillDeck()
{
	for (auto& Cards : ASCToCards)
	{
		TArray<TObjectPtr<UCardWidget>>& Deck = Cards.Value.Deck;
		TArray<TObjectPtr<UCardWidget>>& Graves = Cards.Value.Graves;
		
		check(Deck.IsEmpty());
		check(!Graves.IsEmpty());
		
		Deck = Cards.Value.Graves;
		Graves.Reset();
	}
}

void UCardLayoutManager::MoveCardToGrave(UCardWidget* CardWidget) const
{
	if (CardWidget)
	{
		FWidgetTransform WidgetTransform = CardWidget->GetRenderTransform();
		WidgetTransform.Translation = GravesCardTranslation;
		CardWidget->SetTargetTransform(WidgetTransform);
	}
}

void UCardLayoutManager::MoveAllCards(const TArray<FAbilitySystemReference>& AbilitySystemReferences)
{
	HandZOrder = 200;
	CurrentHands.Reset();

	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemReference.AbilitySystemComponent);
		if (!CharacterCards)
		{
			continue;
		}

		for (UCardWidget* CardInDeck : CharacterCards->Deck)
		{
			// 사용한 카드는 로직상으로는 아직 배열에 남아있지만 View 상으로는 무덤 위치로 가있어야 합니다.
			// 따라서 CurrentCardContainer가 Hand인 경우만 아래 위치 이동 로직을 수행합니다.
			FWidgetTransform WidgetTransform = CardInDeck->GetRenderTransform();
			WidgetTransform.Translation = NextCardTranslation;
			CardInDeck->SetTargetTransform(WidgetTransform);

			if (UCanvasPanelSlot* LastDeckCardSlot = Cast<UCanvasPanelSlot>(CardInDeck->Slot))
			{
				LastDeckCardSlot->SetZOrder(DeckZOrder);
			}
		}

		if (!CharacterCards->Deck.IsEmpty())
		{
			if (UCanvasPanelSlot* LastDeckCardSlot = Cast<UCanvasPanelSlot>(CharacterCards->Deck.Last()->Slot))
			{
				LastDeckCardSlot->SetZOrder(DeckZOrder + 1);
			}
		}

		NextCardTranslation.X += PaddingDeckAndHand;

		for (uint8 HandIndex = 0; HandIndex < CharacterCards->Hands.Num(); ++HandIndex)
		{
			if (UCardWidget* CardInHand = CharacterCards->Hands[HandIndex])
			{
				if (CardInHand->GetCurrentCardContainer() == ECardContainer::Hand)
				{
					FWidgetTransform WidgetTransform = CardInHand->GetRenderTransform();
					WidgetTransform.Translation = NextCardTranslation;
					CardInHand->SetTargetTransform(WidgetTransform);
				}

				if (UCanvasPanelSlot* HandCardSlot = Cast<UCanvasPanelSlot>(CardInHand->Slot))
				{
					HandCardSlot->SetZOrder(HandZOrder++);
				}

				// 핸드의 마지막 장이면 DeckAndHand로, 아니라면 HandAndHand로 사이 공간을 띄워줍니다.
				NextCardTranslation.X += HandIndex == CharacterCards->Hands.Num() - 1 ? PaddingDeckAndHand : PaddingHandAndHand;
				CurrentHands.Add(CardInHand);
			}
		}
	}

	NextCardTranslation = FirstCardTranslation;
}

bool UCardLayoutManager::AreAllDecksFull() const
{
	for (const auto& Cards : ASCToCards)
	{
		if (Cards.Value.Deck.Num() != MAX_DECK_COUNT)
		{
			return false;
		}
	}
	return true;
}

bool UCardLayoutManager::AreAllDecksEmpty() const
{
	for (const auto& Cards : ASCToCards)
	{
		if (!Cards.Value.Deck.IsEmpty())
		{
			return false;
		}
	}
	return true;
}

const TArray<TObjectPtr<UCardWidget>>& UCardLayoutManager::GetCurrentHands() const
{
	return CurrentHands;
}

int32 UCardLayoutManager::GetCurrentHandsNum() const
{
	return CurrentHands.Num();
}

int32 UCardLayoutManager::FindCurrentHandIndex(UCardWidget* CardWidget) const
{
	return CurrentHands.Find(CardWidget);
}
*/
