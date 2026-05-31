// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardLayoutManager.h"

#include "Lethe/Lethe.h"
#include "Lethe/Util.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Actor/Card/CardActor.h"

void UCardLayoutManager::Initialize(const FVector2D& CardSize, const FTransform& InLayoutTransform)
{
	LayoutTransform = InLayoutTransform;
	PaddingDeckAndHand += CardSize.X;
	PaddingHandAndHand += CardSize.X;
	FirstCardLocation.X += CardSize.X / 2.f;
	FirstCardLocation.Y -= CardSize.Y / 2.f;
	NextCardLocation.X += CardSize.X / 2.f;
	NextCardLocation.Y -= CardSize.Y / 2.f;
	GravesCardLocation.X += CardSize.X / 2.f;
	GravesCardLocation.Y -= CardSize.Y / 2.f;

	ASCToCards.Reserve(PLAYER_CHARACTER_NUMBER);
}

void UCardLayoutManager::AddCardToDeck(ACardActor* CardActor)
{
	if (CardActor)
	{
		if (ULetheAbilitySystemComponent* LetheASC = CardActor->GetOwnerASC())
		{
			FCharacterCards& CharacterCards = ASCToCards.FindOrAdd(LetheASC);
			CharacterCards.Deck.Add(CardActor);
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

ACardActor* UCardLayoutManager::GetTopCardFromDeck(ULetheAbilitySystemComponent* OwnerASC) const
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
			if (ACardActor* DrawnCard = CharacterCards->Deck.Pop(EAllowShrinking::No))
			{
				CharacterCards->Hands.Add(DrawnCard);
				DrawnCard->SetCardContainer(ECardContainer::Hand);
				return true;
			}
		}
	}
	return false;
}

void UCardLayoutManager::AddCardToGrave(ACardActor* CardActor)
{
	if (CardActor && CardActor->GetCurrentCardContainer() != ECardContainer::Grave)
	{
		if (const ULetheAbilitySystemComponent* LetheASC = CardActor->GetOwnerASC())
		{
			if (FCharacterCards* CharacterCards = ASCToCards.Find(LetheASC))
			{
				CharacterCards->Graves.AddUnique(CardActor);
			}
			MoveCardToGraves(CardActor);
			CardActor->SetCardContainer(ECardContainer::Grave);
		}
	}
}

void UCardLayoutManager::AddAllHandsToGrave()
{
	for (auto& Cards : ASCToCards)
	{
		for (ACardActor* Hand : Cards.Value.Hands)
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
		TArray<TObjectPtr<ACardActor>>& Deck = Cards.Value.Deck;
		TArray<TObjectPtr<ACardActor>>& Graves = Cards.Value.Graves;

		Deck.Append(Graves);
		Graves.Reset();
	}
}

void UCardLayoutManager::MoveCardToGraves(ACardActor* CardActor) const
{
	if (CardActor)
	{
		FTransform ActorTransform = CardActor->GetActorTransform();
		ActorTransform.SetLocation(LayoutTransform.TransformPosition(GravesCardLocation));
		CardActor->SetTargetTransform(ActorTransform);
	}
}

void UCardLayoutManager::MoveAllCards(const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& AbilitySystemComponents)
{
	CurrentHands.Reset();

	for (ULetheAbilitySystemComponent* AbilitySystemComponent : AbilitySystemComponents)
	{
		FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemComponent);
		if (!CharacterCards)
		{
			continue;
		}

		for (ACardActor* CardInDeck : CharacterCards->Deck)
		{
			FTransform ActorTransform = CardInDeck->GetActorTransform();
			ActorTransform.SetLocation(LayoutTransform.TransformPosition(NextCardLocation));
			CardInDeck->SetTargetTransform(ActorTransform);
		}

		NextCardLocation.X += PaddingDeckAndHand;

		for (uint8 HandIndex = 0; HandIndex < CharacterCards->Hands.Num(); ++HandIndex)
		{
			if (ACardActor* CardInHand = CharacterCards->Hands[HandIndex])
			{
				if (CardInHand->GetCurrentCardContainer() == ECardContainer::Hand)
				{
					FTransform ActorTransform = CardInHand->GetActorTransform();
					ActorTransform.SetLocation(LayoutTransform.TransformPosition(NextCardLocation));
					CardInHand->SetTargetTransform(ActorTransform);
				}

				// 핸드의 마지막 장이면 DeckAndHand로, 아니라면 HandAndHand로 사이 공간을 띄워줍니다.
				NextCardLocation.X += HandIndex == CharacterCards->Hands.Num() - 1 ? PaddingDeckAndHand : PaddingHandAndHand;
				CurrentHands.Add(CardInHand);
			}
		}
	}

	NextCardLocation = FirstCardLocation;
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

const TArray<TObjectPtr<ACardActor>>& UCardLayoutManager::GetCurrentHands() const
{
	return CurrentHands;
}

int32 UCardLayoutManager::GetCurrentHandsNum() const
{
	return CurrentHands.Num();
}

int32 UCardLayoutManager::FindCurrentHandIndex(ACardActor* CardActor) const
{
	return CurrentHands.Find(CardActor);
}
