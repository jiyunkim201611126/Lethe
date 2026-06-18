// Copyright JETBLU, Inc. All Rights Reserved.

#include "PlayerGASManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"

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
	ALetheCharacterBase* OwnerCharacter = GetOwner<ALetheCharacterBase>();
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController);
	if (!OwnerCharacter || !LethePlayerController)
	{
		return;
	}
	
	ULetheWidgetController* WidgetController = LethePlayerController->InitPlayerUI(AbilitySystemComponent, AttributeSet, PlayerAttributeSet);
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
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	AbilitySystemComponent->SetLooseGameplayTagCount(LetheGameplayTags.State_Character_CanAct, 0);
	
	if (NewPhase == EPhaseState::PlayerMovePhase || NewPhase == EPhaseState::PlayerTurnPhase)
	{
		AbilitySystemComponent->AddLooseGameplayTag(LetheGameplayTags.State_Character_CanAct);
	}
}

void UPlayerGASManagerComponent::OnPlanPhaseStarted() const
{
	Super::OnPlanPhaseStarted();
	
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
