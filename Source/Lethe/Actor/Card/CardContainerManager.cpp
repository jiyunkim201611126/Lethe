// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardContainerManager.h"

#include "DeckBoxes.h"
#include "Lethe/Lethe.h"
#include "Lethe/Util.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Actor/Card/CardActor.h"

void FCharacterCards::SortDeckPreviewHands()
{
	Algo::Sort(DeckPreviewHands, [](const ACardActor* CardA, const ACardActor* CardB)
	{
		if (CardA && CardB)
		{
			const uint64 CardAId = CardA->GetSavedCard().CardId;
			const uint64 CardBId = CardB->GetSavedCard().CardId;
			return CardAId < CardBId;
		}
		return false;
	});
}



void UCardContainerManager::Initialize(const TArray<TWeakObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents, ADeckBoxes* InDeckBoxes)
{
	AbilitySystemComponents = InAbilitySystemComponents;
	ASCToCards.Reserve(PLAYER_CHARACTER_NUMBER);
	CurrentHands.Reserve(10);
	DeckBoxes = InDeckBoxes;
}

void UCardContainerManager::AddCardToDeck(ACardActor* CardActor)
{
	if (CardActor)
	{
		if (ULetheAbilitySystemComponent* CardOwnerASC = CardActor->GetOwnerASC())
		{
			FCharacterCards& CharacterCards = ASCToCards.FindOrAdd(CardOwnerASC);
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

bool UCardContainerManager::AddCardToHand(ULetheAbilitySystemComponent* OwnerASC)
{
	if (OwnerASC)
	{
		FCharacterCards* CharacterCards = ASCToCards.Find(OwnerASC);
		if (CharacterCards && !CharacterCards->Deck.IsEmpty())
		{
			if (ACardActor* DrawnCard = CharacterCards->Deck.Pop(EAllowShrinking::No))
			{
				CharacterCards->Hands.Add(DrawnCard);
				DrawnCard->SetCardContainer(ECardContainer::Hands);
				return true;
			}
		}
	}
	return false;
}

void UCardContainerManager::AddCardToGraveyard(ACardActor* CardActor)
{
	if (CardActor && CardActor->GetCurrentCardContainer() != ECardContainer::Graveyard)
	{
		if (const ULetheAbilitySystemComponent* CardOwnerASC = CardActor->GetOwnerASC())
		{
			if (FCharacterCards* CharacterCards = ASCToCards.Find(CardOwnerASC))
			{
				CharacterCards->Graveyard.AddUnique(CardActor);
				const int32 HandIndex = CharacterCards->Hands.IndexOfByKey(CardActor);
				if (HandIndex != INDEX_NONE)
				{
					CharacterCards->Hands[HandIndex] = nullptr;
				}
			}
			CardActor->SetCardContainer(ECardContainer::Graveyard);
		}
	}
}

void UCardContainerManager::AddAllHandsToGraveyard()
{
	for (auto& Cards : ASCToCards)
	{
		for (ACardActor* Hand : Cards.Value.Hands)
		{
			AddCardToGraveyard(Hand);
		}
		Cards.Value.Hands.Reset();
	}
}

void UCardContainerManager::RefillDeck()
{
	for (auto& Cards : ASCToCards)
	{
		TArray<TObjectPtr<ACardActor>>& Deck = Cards.Value.Deck;
		TArray<TObjectPtr<ACardActor>>& Graveyard = Cards.Value.Graveyard;

		Deck.Append(Graveyard);
		Graveyard.Reset();
	}
}

void UCardContainerManager::MoveAllCards()
{
	// 드로우는 캐릭터 순서에 관계 없이 진행되므로, 캐릭터 순서에 맞춰 정렬된 핸드를 얻기 위해 여기서 캐싱 로직을 수행합니다.
	CurrentHands.Reset();

	TArray<int32> OutCurrentHandCounts;
	GetCurrentHandCounts(OutCurrentHandCounts);
	DeckBoxes->UpdateLocations(OutCurrentHandCounts);

	TArray<FVector> OutDeckLocations;
	DeckBoxes->GetDeckLocations(OutDeckLocations);
	
	for (int32 Index = 0; Index < AbilitySystemComponents.Num(); ++Index)
	{
		if (!AbilitySystemComponents.IsValidIndex(Index) || !OutDeckLocations.IsValidIndex(Index))
		{
			return;
		}
		
		FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemComponents[Index]);
		if (!CharacterCards)
		{
			continue;
		}

		const FVector DeckLocation = OutDeckLocations[Index];
		
		FVector DeckCardLocation = DeckLocation + DeckFirstCardLocation;
		for (ACardActor* CardInDeck : CharacterCards->Deck)
		{
			if (CardInDeck)
			{
				DeckCardLocation += FVector(DeckCardXOffset, 0.f, 0.f);
				CardInDeck->SetActorLocation(DeckCardLocation);
				CardInDeck->SetActorRotation(FRotator(0.f, -90.f, 0.f));
			}
		}

		FVector HandCardLocation = DeckLocation + HandFirstCardLocation;
		TArray<ACardActor*> SelectedHands = CharacterCards->DeckPreviewHands.IsEmpty() ? CharacterCards->Hands : CharacterCards->DeckPreviewHands;
		for (ACardActor* CardInHand : SelectedHands)
		{
			// 카드 사용 시 해당 위치가 nullptr로 대체되기 때문에, nullptr도 배열에 넣어주고 위치 벡터도 더해줍니다. 
			CurrentHands.Add(CardInHand);
			HandCardLocation += FVector(HandCardXOffset, 0.f, 0.f);
			
			if (CardInHand)
			{
				CardInHand->SetActorLocation(HandCardLocation);
				CardInHand->SetActorRotation(FRotator(0.f, 0.f, 0.f));
			}
		}

		FVector GraveyardCardLocation = DeckLocation + GraveyardFirstCardLocation;
		for (ACardActor* CardInGraveyard : CharacterCards->Graveyard)
		{
			if (CardInGraveyard)
			{
				GraveyardCardLocation += GraveyardCardOffset;
				CardInGraveyard->SetActorLocation(GraveyardCardLocation);
				CardInGraveyard->SetActorRotation(FRotator(0.f, 180.f, -60.f));
			}
		}
	}
}

void UCardContainerManager::PreviewDeck(ULetheAbilitySystemComponent* DeckOwnerASC)
{
	if (!DeckOwnerASC)
	{
		return;
	}

	// 카드 Id 순서대로(물리, 마법, 보조 순서) 보여주기 위해 핸드를 한 번 정렬합니다.
	if (FCharacterCards* CharacterCards = ASCToCards.Find(DeckOwnerASC))
	{
		CharacterCards->DeckPreviewHands = CharacterCards->Deck;
		CharacterCards->SortDeckPreviewHands();
	}
	
	MoveAllCards();
}

void UCardContainerManager::StopPreviewDeck(const bool bShouldMoveCards)
{
	for (auto& Cards : ASCToCards)
	{
		Cards.Value.DeckPreviewHands.Reset();
	}

	if (bShouldMoveCards)
	{
		MoveAllCards();
	}
}

bool UCardContainerManager::AreAllDecksFull() const
{
	if (ASCToCards.Num() != AbilitySystemComponents.Num())
	{
		return false;
	}
	
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

void UCardContainerManager::GetCurrentHandCounts(TArray<int32>& OutHandCounts)
{
	OutHandCounts.Reset();
	OutHandCounts.Reserve(AbilitySystemComponents.Num());

	for (const auto& AbilitySystemComponent : AbilitySystemComponents)
	{
		const FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemComponent);
		if (!CharacterCards)
		{
			continue;
		}

		const int32 HandCount = CharacterCards->Hands.Num();
		const int32 PreviewHandCount = CharacterCards->DeckPreviewHands.Num();
		OutHandCounts.Add(HandCount + PreviewHandCount);
	}
}
