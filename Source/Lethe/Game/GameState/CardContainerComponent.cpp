// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardContainerComponent.h"

#include "Lethe/Lethe.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Util.h"
#include "Lethe/SaveGame/SavedCardTypes.h"

UCardContainerComponent::UCardContainerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCardContainerComponent::Initialize(const TArray<FGameplayTag>& InCharacterOrder, const TMap<FGameplayTag, FSavedCharacterDeck>& EquippedDecks, const int32 InitialShuffleSeed)
{
	CharacterOrder = InCharacterOrder;
	Instances.Reset();
	ZonesByCharacter.Reset();
	ShuffleStream.Initialize(InitialShuffleSeed);

	for (const FGameplayTag& CharacterTag : CharacterOrder)
	{
		FCharacterCardZones& Zones = ZonesByCharacter.FindOrAdd(CharacterTag);
		const FSavedCharacterDeck* SavedDeck = EquippedDecks.Find(CharacterTag);
		if (!SavedDeck)
		{
			LETHE_LOG(LogLetheGameState, Warning, "장착 덱이 없는 캐릭터입니다: %s", *CharacterTag.ToString());
			continue;
		}

		Zones.Deck.Reserve(SavedDeck->Deck.Num());
		for (const FSavedCard& SavedCard : SavedDeck->Deck)
		{
			FCardInstance& Instance = Instances.AddDefaulted_GetRef();
			Instance.InstanceId = Instances.Num() - 1;
			Instance.CardId = SavedCard.CardId;
			Instance.CardLevel = SavedCard.CardLevel;
			Instance.CharacterTag = CharacterTag;

			Zones.Deck.Add(Instance.InstanceId);
		}
	}

	bInitialized = true;
	ShuffleAllDecks();
	BroadcastChanged();
}

bool UCardContainerComponent::IsInitialized() const
{
	return bInitialized;
}

void UCardContainerComponent::HandleTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState, const bool bIsBattlePhase)
{
	if (!bInitialized)
	{
		return;
	}

	if (OldTurnPhaseState == ETurnPhaseState::PlayerTurnPhase)
	{
		// 플레이어 턴이 끝나면 핸드의 모든 카드를 버립니다.
		DiscardAllHands();
	}

	if (NewTurnPhaseState == ETurnPhaseState::DrawPhase && AreAllDecksEmpty())
	{
		// 드로우를 시작하려는데 덱이 모두 비어있다면 무덤을 덱으로 되돌립니다.
		RefillAllDecks();
		ShuffleAllDecks();
	}

	if (!bIsBattlePhase)
	{
		// 전투 중이 아니라면 모든 카드를 덱으로 되돌립니다.
		DiscardAllHands();
		RefillAllDecks();
		ShuffleAllDecks();
	}

	BroadcastChanged();
}

bool UCardContainerComponent::DrawTopCard(const FGameplayTag& CharacterTag, int32& OutInstanceId)
{
	OutInstanceId = INDEX_NONE;

	FCharacterCardZones* Zones = FindZones(CharacterTag);
	if (!Zones || Zones->Deck.IsEmpty())
	{
		return false;
	}

	OutInstanceId = Zones->Deck.Pop(EAllowShrinking::No);
	Zones->HandSlots.Add(OutInstanceId);
	BroadcastChanged();

	if (GetHandSlotCount() >= MAX_HAND_SLOT_COUNT)
	{
		OnHandFull.Broadcast();
	}
	return true;
}

bool UCardContainerComponent::MoveToGraveyard(const int32 InstanceId)
{
	const FCardInstance* Instance = FindInstance(InstanceId);
	FCharacterCardZones* Zones = Instance ? FindZones(Instance->CharacterTag) : nullptr;
	if (!Zones)
	{
		return false;
	}

	const int32 HandSlotIndex = Zones->HandSlots.IndexOfByKey(InstanceId);
	if (HandSlotIndex == INDEX_NONE)
	{
		return false;
	}

	// 슬롯은 유지하고 값만 비웁니다.
	Zones->HandSlots[HandSlotIndex] = INDEX_NONE;
	Zones->Graveyard.AddUnique(InstanceId);
	BroadcastChanged();
	return true;
}

void UCardContainerComponent::DiscardAllHands()
{
	for (auto& Pair : ZonesByCharacter)
	{
		FCharacterCardZones& Zones = Pair.Value;
		for (const int32 InstanceId : Zones.HandSlots)
		{
			if (InstanceId != INDEX_NONE)
			{
				Zones.Graveyard.AddUnique(InstanceId);
			}
		}
		Zones.HandSlots.Reset();
	}
}

void UCardContainerComponent::RefillAllDecks()
{
	for (auto& Pair : ZonesByCharacter)
	{
		FCharacterCardZones& Zones = Pair.Value;
		Zones.Deck.Append(Zones.Graveyard);
		Zones.Graveyard.Reset();
	}
}

void UCardContainerComponent::ShuffleAllDecks()
{
	// 캐릭터 순서대로 셔플해야 같은 시드에서 같은 결과가 나옵니다. TMap 순회 순서에 의존하지 않습니다.
	for (const FGameplayTag& CharacterTag : CharacterOrder)
	{
		if (FCharacterCardZones* Zones = FindZones(CharacterTag))
		{
			ArrayShuffle::ShuffleWithSeed(Zones->Deck, ShuffleStream);
		}
	}
}

const FCardInstance* UCardContainerComponent::FindInstance(const int32 InstanceId) const
{
	return Instances.IsValidIndex(InstanceId) ? &Instances[InstanceId] : nullptr;
}

ECardZone UCardContainerComponent::GetZone(const int32 InstanceId) const
{
	const FCardInstance* Instance = FindInstance(InstanceId);
	const FCharacterCardZones* Zones = Instance ? GetZones(Instance->CharacterTag) : nullptr;
	if (!Zones)
	{
		return ECardZone::None;
	}

	if (Zones->Deck.Contains(InstanceId))
	{
		return ECardZone::Deck;
	}
	if (Zones->HandSlots.Contains(InstanceId))
	{
		return ECardZone::Hand;
	}
	if (Zones->Graveyard.Contains(InstanceId))
	{
		return ECardZone::Graveyard;
	}
	return ECardZone::None;
}

const FCharacterCardZones* UCardContainerComponent::GetZones(const FGameplayTag& CharacterTag) const
{
	return ZonesByCharacter.Find(CharacterTag);
}

FCharacterCardZones* UCardContainerComponent::FindZones(const FGameplayTag& CharacterTag)
{
	return ZonesByCharacter.Find(CharacterTag);
}

const TArray<FGameplayTag>& UCardContainerComponent::GetCharacterOrder() const
{
	return CharacterOrder;
}

const TArray<FCardInstance>& UCardContainerComponent::GetInstances() const
{
	return Instances;
}

void UCardContainerComponent::GetOrderedHandSlots(TArray<int32>& OutInstanceIds) const
{
	OutInstanceIds.Reset();
	for (const FGameplayTag& CharacterTag : CharacterOrder)
	{
		if (const FCharacterCardZones* Zones = GetZones(CharacterTag))
		{
			OutInstanceIds.Append(Zones->HandSlots);
		}
	}
}

int32 UCardContainerComponent::GetHandSlotCount() const
{
	int32 Count = 0;
	for (const auto& Pair : ZonesByCharacter)
	{
		Count += Pair.Value.HandSlots.Num();
	}
	return Count;
}

bool UCardContainerComponent::AreAllDecksEmpty() const
{
	for (const auto& Pair : ZonesByCharacter)
	{
		if (!Pair.Value.Deck.IsEmpty())
		{
			return false;
		}
	}
	return true;
}

void UCardContainerComponent::Capture(FCardContainerSnapshot& OutSnapshot) const
{
	OutSnapshot.CharacterOrder = CharacterOrder;
	OutSnapshot.Instances = Instances;
	OutSnapshot.ZonesByCharacter = ZonesByCharacter;
	OutSnapshot.ShuffleStreamSeed = ShuffleStream.GetCurrentSeed();
}

void UCardContainerComponent::Restore(const FCardContainerSnapshot& InSnapshot)
{
	CharacterOrder = InSnapshot.CharacterOrder;
	Instances = InSnapshot.Instances;
	ZonesByCharacter = InSnapshot.ZonesByCharacter;
	ShuffleStream.Initialize(InSnapshot.ShuffleStreamSeed);
	bInitialized = true;
	BroadcastChanged();
}

void UCardContainerComponent::BroadcastChanged() const
{
	OnContainerChanged.Broadcast();
}
