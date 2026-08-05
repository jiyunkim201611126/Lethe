// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardContainerManager.h"

#include "DeckBoxes.h"
#include "Lethe/Lethe.h"
#include "Lethe/Util.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Actor/Card/CardActor.h"

void FCharacterCards::SortDeckPreviewCards()
{
	Algo::Sort(DeckPreviewCards, [](const ACardActor* CardA, const ACardActor* CardB)
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
	CurrentHandSlots.Reserve(10);
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

bool UCardContainerManager::AddCardToHandSlot(ULetheAbilitySystemComponent* OwnerASC)
{
	if (OwnerASC)
	{
		FCharacterCards* CharacterCards = ASCToCards.Find(OwnerASC);
		if (CharacterCards && !CharacterCards->Deck.IsEmpty())
		{
			if (ACardActor* DrawnCard = CharacterCards->Deck.Pop(EAllowShrinking::No))
			{
				FHandSlot& NewHandSlot = CharacterCards->HandSlots.AddDefaulted_GetRef();
				NewHandSlot.SetCard(DrawnCard);
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
				for (FHandSlot& HandSlot : CharacterCards->HandSlots)
				{
					if (HandSlot.GetCard() == CardActor)
					{
						HandSlot.Clear();
						break;
					}
				}
			}
			CardActor->SetCardContainer(ECardContainer::Graveyard);
		}
	}
}

void UCardContainerManager::AddAllHandSlotsToGraveyard()
{
	for (auto& Cards : ASCToCards)
	{
		for (FHandSlot& HandSlot : Cards.Value.HandSlots)
		{
			AddCardToGraveyard(HandSlot.GetCard());
		}
		Cards.Value.HandSlots.Reset();
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
	// 드로우는 캐릭터 순서에 관계 없이 진행되므로, 캐릭터 순서에 맞춰 정렬된 핸드 슬롯을 얻기 위해 여기서 캐싱 로직을 수행합니다.
	CurrentHandSlots.Reset();

	TArray<int32> OutCurrentHandSlotCounts;
	GetCurrentHandSlotCounts(OutCurrentHandSlotCounts);
	DeckBoxes->UpdateLocations(OutCurrentHandSlotCounts);

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

		FVector HandSlotCardLocation = DeckLocation + HandSlotFirstCardLocation;
		if (!CharacterCards->DeckPreviewCards.IsEmpty())
		{
			for (ACardActor* CardInPreview : CharacterCards->DeckPreviewCards)
			{
				FHandSlot& PreviewHandSlot = CurrentHandSlots.AddDefaulted_GetRef();
				PreviewHandSlot.SetCard(CardInPreview);
				HandSlotCardLocation += FVector(HandSlotCardXOffset, 0.f, 0.f);

				if (CardInPreview)
				{
					CardInPreview->SetActorLocation(HandSlotCardLocation);
					CardInPreview->SetActorRotation(FRotator(0.f, 0.f, 0.f));
				}
			}
		}
		else
		{
			for (const FHandSlot& HandSlot : CharacterCards->HandSlots)
			{
				// 카드를 사용해도 슬롯은 유지되므로, 비어 있는 슬롯도 CurrentHandSlots에 포함합니다.
				CurrentHandSlots.Add(HandSlot);
				HandSlotCardLocation += FVector(HandSlotCardXOffset, 0.f, 0.f);

				if (!HandSlot.IsEmpty())
				{
					ACardActor* CardInHand = HandSlot.GetCard();
					CardInHand->SetActorLocation(HandSlotCardLocation);
					CardInHand->SetActorRotation(FRotator(0.f, 0.f, 0.f));
				}
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

	// 카드 Id 순서대로(물리, 마법, 보조 순서) 보여주기 위해 덱 미리보기 카드를 한 번 정렬합니다.
	if (FCharacterCards* CharacterCards = ASCToCards.Find(DeckOwnerASC))
	{
		CharacterCards->DeckPreviewCards = CharacterCards->Deck;
		CharacterCards->SortDeckPreviewCards();
	}
	
	MoveAllCards();
}

void UCardContainerManager::StopPreviewDeck(const bool bShouldMoveCards)
{
	for (auto& Cards : ASCToCards)
	{
		Cards.Value.DeckPreviewCards.Reset();
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

const TArray<FHandSlot>& UCardContainerManager::GetCurrentHandSlots() const
{
	return CurrentHandSlots;
}

int32 UCardContainerManager::GetCurrentHandSlotCount() const
{
	return CurrentHandSlots.Num();
}

int32 UCardContainerManager::FindCurrentHandSlotIndex(const ACardActor* CardActor) const
{
	for (int32 Index = 0; Index < CurrentHandSlots.Num(); ++Index)
	{
		if (CurrentHandSlots[Index].GetCard() == CardActor)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void UCardContainerManager::GetCurrentHandSlotCounts(TArray<int32>& OutHandSlotCounts)
{
	OutHandSlotCounts.Reset();
	OutHandSlotCounts.Reserve(AbilitySystemComponents.Num());

	for (const auto& AbilitySystemComponent : AbilitySystemComponents)
	{
		const FCharacterCards* CharacterCards = ASCToCards.Find(AbilitySystemComponent);
		if (!CharacterCards)
		{
			continue;
		}

		const int32 HandSlotCount = CharacterCards->HandSlots.Num();
		const int32 PreviewCardCount = CharacterCards->DeckPreviewCards.Num();
		OutHandSlotCounts.Add(HandSlotCount + PreviewCardCount);
	}
}
