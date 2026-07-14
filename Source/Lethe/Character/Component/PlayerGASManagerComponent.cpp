// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerGASManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Data/PhaseData.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "Lethe/UI/Framework/Policy/BattleUIFeature.h"

UPlayerGASManagerComponent::UPlayerGASManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerGASManagerComponent::SetPlayerAttributeSet(UPlayerAttributeSet* InPlayerAttributeSet)
{
	PlayerAttributeSet = InPlayerAttributeSet;
}

void UPlayerGASManagerComponent::InitUI(const TArray<UUserWidget*>& AttributeWidgets)
{
	// PlayerController가 빙의하는 캐릭터가 아니기 때문에 라이브러리 함수로 가져옵니다.
	ALetheCharacterBase* OwnerCharacter = GetOwner<ALetheCharacterBase>();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	if (!OwnerCharacter || !PlayerController)
	{
		return;
	}

	const ULetheUIManagerSubsystem* UIManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULetheUIManagerSubsystem>();
	if (!UIManagerSubsystem)
	{
		return;
	}

	const TSubclassOf<UBattleUIFeature> LoadedBattleUIFeatureClass = BattleUIFeatureClass.LoadSynchronous();
	UBattleUIFeature* BattleUIFeature = UIManagerSubsystem->GetOrCreateUIFeature<UBattleUIFeature>(LoadedBattleUIFeatureClass);
	if (!BattleUIFeature)
	{
		return;
	}

	BattleUIFeature->InitPlayerBattleUI(PlayerController, AbilitySystemComponent, AttributeSet, PlayerAttributeSet);
	ULetheWidgetController* WidgetController = BattleUIFeature->CreatePlayerAttributeWidgetController(PlayerController, AbilitySystemComponent, AttributeSet, PlayerAttributeSet);
	
	for (UUserWidget* AttributeWidget : AttributeWidgets)
	{
		CastChecked<ULetheUserWidget>(AttributeWidget)->SetWidgetController(WidgetController);
	}

	// UDeckManagerSubsystem에서 Owner의 EquippedDeck을 가져옵니다.
	const FGameplayTag& CharacterTag = OwnerCharacter->GetCharacterTag();
	if (UDeckManagerSubsystem* DeckManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UDeckManagerSubsystem>())
	{
		const TMap<FGameplayTag, FSavedCharacterDeck>& EquippedDecks = DeckManagerSubsystem->GetEquippedDecks();
		if (const FSavedCharacterDeck* CharacterDeck = EquippedDecks.Find(CharacterTag))
		{
			// Equipped Deck들을 실제 Ability로 부여합니다.
			AddCharacterAbilities(CharacterDeck->Deck);
		}
	}
}

void UPlayerGASManagerComponent::OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase) const
{
	if (NewPhase == EPhaseState::EnemyPlanningPhase)
	{
		OnPlanPhaseStarted();
		ApplyTurnStartRecovery();
	}

	if (NewPhase == EPhaseState::DrawPhase)
	{
		ApplyTurnStartRecovery();
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	AbilitySystemComponent->SetLooseGameplayTagCount(LetheGameplayTags.State_Character_CanAct, 0);
	
	if (NewPhase == EPhaseState::PlayerMovePhase || NewPhase == EPhaseState::PlayerTurnPhase)
	{
		AbilitySystemComponent->AddLooseGameplayTag(LetheGameplayTags.State_Character_CanAct);
	}
}

void UPlayerGASManagerComponent::ApplyTurnStartRecovery() const
{
	if (!TurnStartRecovery)
	{
		return;
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	
	const FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(TurnStartRecovery, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		SpecHandle.Data->SetSetByCallerMagnitude(LetheGameplayTags.Attribute_Vital_ManaRecovery, AbilitySystemComponent->GetNumericAttribute(UPlayerAttributeSet::GetManaRecoveryAttribute()));
		SpecHandle.Data->SetSetByCallerMagnitude(LetheGameplayTags.Attribute_Vital_CostRecovery, AbilitySystemComponent->GetNumericAttribute(UPlayerAttributeSet::GetCostRecoveryAttribute()));
		SpecHandle.Data->SetSetByCallerMagnitude(LetheGameplayTags.Attribute_Vital_MoveRangeRecovery, AbilitySystemComponent->GetNumericAttribute(ULetheAttributeSet::GetMoveRangeRecoveryAttribute()));
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
	}
}
