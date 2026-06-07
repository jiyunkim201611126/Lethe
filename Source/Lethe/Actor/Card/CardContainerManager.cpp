// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardContainerManager.h"

#include "DeckBoxes.h"
#include "Lethe/Lethe.h"
#include "Lethe/Util.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Actor/Card/CardActor.h"

void UCardContainerManager::Initialize(const TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents, ADeckBoxes* InDeckBoxes)
{
	AbilitySystemComponents = InAbilitySystemComponents;
	ASCToCards.Reserve(PLAYER_CHARACTER_NUMBER);
	DeckBoxes = InDeckBoxes;
}

void UCardContainerManager::AddCardToDeck(ACardActor* CardActor)
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

void UCardContainerManager::ShuffleDeck()
{
	for (auto& Cards : ASCToCards)
	{
		FRandomStream Stream(12345);
		ArrayShuffle::ShuffleWithSeed(Cards.Value.Deck, Stream);
	}
}

ACardActor* UCardContainerManager::GetTopCardFromDeck(ULetheAbilitySystemComponent* OwnerASC) const
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

bool UCardContainerManager::TryDraw(ULetheAbilitySystemComponent* OwnerASC)
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

void UCardContainerManager::AddCardToGrave(ACardActor* CardActor)
{
	if (CardActor && CardActor->GetCurrentCardContainer() != ECardContainer::Grave)
	{
		if (const ULetheAbilitySystemComponent* LetheASC = CardActor->GetOwnerASC())
		{
			if (FCharacterCards* CharacterCards = ASCToCards.Find(LetheASC))
			{
				CharacterCards->Graves.AddUnique(CardActor);
			}
			CardActor->SetCardContainer(ECardContainer::Grave);
		}
	}
}

void UCardContainerManager::AddAllHandsToGrave()
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

void UCardContainerManager::RefillDeck()
{
	for (auto& Cards : ASCToCards)
	{
		TArray<TObjectPtr<ACardActor>>& Deck = Cards.Value.Deck;
		TArray<TObjectPtr<ACardActor>>& Graves = Cards.Value.Graves;

		Deck.Append(Graves);
		Graves.Reset();
	}
}

void UCardContainerManager::MoveAllCards()
{
	// 드로우는 캐릭터 순서에 관계 없이 진행되므로, 캐릭터 순서에 맞춰 정렬된 핸드를 얻기 위해 여기서 캐싱 로직을 수행합니다.
	CurrentHands.Reset();

	TArray<int32> CurrentHandCounts;
	GetCurrentHandCounts(CurrentHandCounts);
	DeckBoxes->UpdateLocations(CurrentHandCounts);

	TArray<FVector> DeckLocations;
	DeckBoxes->GetDeckLocations(DeckLocations);
	
	for (int32 Index = 0; Index < AbilitySystemComponents.Num(); ++Index)
	{
		if (!AbilitySystemComponents.IsValidIndex(Index) || !DeckLocations.IsValidIndex(Index))
		{
			return;
		}
		
		FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemComponents[Index]);
		if (!CharacterCards)
		{
			continue;
		}

		const FVector DeckLocation = DeckLocations[Index];
		
		FVector DeckCardLocation = DeckLocation + DeckFirstCardLocation;
		for (ACardActor* CardInDeck : CharacterCards->Deck)
		{
			DeckCardLocation += FVector(DeckCardXOffset, 0.f, 0.f);
			CardInDeck->SetActorLocation(DeckCardLocation);
			CardInDeck->SetActorRotation(FRotator(0.f, -90.f, 0.f));
		}

		FVector HandCardLocation = DeckLocation + HandFirstCardLocation;
		for (int32 HandIndex = 0; HandIndex < CharacterCards->Hands.Num(); ++HandIndex)
		{
			if (ACardActor* CardInHand = CharacterCards->Hands[HandIndex])
			{
				CurrentHands.Add(CardInHand);

				HandCardLocation += FVector(HandCardXOffset, 0.f, 0.f);
				CardInHand->SetActorLocation(HandCardLocation);
				CardInHand->SetActorRotation(FRotator(0.f, 0.f, 0.f));
			}
		}

		FVector GravesCardLocation = DeckLocation + GravesFirstCardLocation;
		for (int32 GraveIndex = 0; GraveIndex < CharacterCards->Graves.Num(); ++GraveIndex)
		{
			if (ACardActor* CardInGrave = CharacterCards->Graves[GraveIndex])
			{
				GravesCardLocation += GravesCardOffset;
				CardInGrave->SetActorLocation(GravesCardLocation);
				CardInGrave->SetActorRotation(FRotator(0.f, 180.f, -60.f));
			}
		}
	}
}

bool UCardContainerManager::AreAllDecksFull() const
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

bool UCardContainerManager::AreAllDecksEmpty() const
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

const TArray<TObjectPtr<ACardActor>>& UCardContainerManager::GetCurrentHands() const
{
	return CurrentHands;
}

int32 UCardContainerManager::GetCurrentHandCount() const
{
	return CurrentHands.Num();
}

int32 UCardContainerManager::FindCurrentHandIndex(ACardActor* CardActor) const
{
	return CurrentHands.Find(CardActor);
}

void UCardContainerManager::GetCurrentHandCounts(TArray<int32>& HandCounts)
{
	HandCounts.Reset();
	HandCounts.Reserve(AbilitySystemComponents.Num());

	for (const auto& AbilitySystemComponent : AbilitySystemComponents)
	{
		
		const FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemComponent);
		if (!CharacterCards)
		{
			continue;
		}
		
		HandCounts.Add(CharacterCards->Hands.Num());
	}
}
