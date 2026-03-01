// Copyright JETBLU, Inc. All Rights Reserved.

#include "GASManagerComponent.h"

#include "LetheAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/HUD/LetheHUD.h"

UGASManagerComponent::UGASManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (TeamSide == ETeamSide::Player)
	{
		if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			LetheGameState->OnChangeTurnStateDelegate.Remove(OnPhaseStateChangedDelegateHandle);
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

void UGASManagerComponent::SetAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent)
{
	AbilitySystemComponent = InAbilitySystemComponent;
}

void UGASManagerComponent::SetAttributeSet(UAttributeSet* InAttributeSet)
{
	AttributeSet = InAttributeSet;
}

UAbilitySystemComponent* UGASManagerComponent::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void UGASManagerComponent::InitAbilityActorInfo(UUserWidget* AttributeWidget)
{
	APawn* OwnerPawn = GetOwner<APawn>();
	
	AbilitySystemComponent->InitAbilityActorInfo(OwnerPawn, OwnerPawn);

	// PlayerController가 빙의하는 캐릭터가 아니기 때문에 라이브러리 함수로 가져옵니다.
	if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
	{
		if (const ALethePlayerController* LethePlayerController = Cast<ALethePlayerController>(PlayerController))
		{
			if (ULetheHUD* LetheHUD = LethePlayerController->GetLetheHUD())
			{
				switch (TeamSide)
				{
				case ETeamSide::Player:
					LetheHUD->InitPlayerUI(PlayerController, GetPawn<APawn>()->GetPlayerState(), AbilitySystemComponent, AttributeSet, AttributeWidget);
					break;
				case ETeamSide::Enemy:
					LetheHUD->InitEnemyUI(PlayerController, AbilitySystemComponent, AttributeSet, AttributeWidget);
					break;
				default:
					break;
				}
			}
		}
	}
	ApplyEffectToSelf(DefaultAttributes, 1.f);

	// UDeckManagerSubsystem에서 Owner의 EquippedDeck을 가져옵니다.
	if (IPlayableCharacterInterface* PlayerCharacter = Cast<IPlayableCharacterInterface>(OwnerPawn))
	{
		const FGameplayTag& CharacterTag = PlayerCharacter->GetCharacterTag();
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
	
	AddCharacterAbilities(StartAbilities);

	if (TeamSide == ETeamSide::Player)
	{
		if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			OnPhaseStateChangedDelegateHandle = LetheGameState->OnChangeTurnStateDelegate.AddUObject(this, &ThisClass::OnPhaseStateChanged);
		}
	}
}

void UGASManagerComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities) const
{
	ULetheAbilitySystemComponent* ASC = CastChecked<ULetheAbilitySystemComponent>(AbilitySystemComponent);
	ASC->AddCharacterAbilities(InAbilities);
}

void UGASManagerComponent::AddCharacterAbilities(const TArray<FSavedCard>& InCards) const
{
	ULetheAbilitySystemComponent* ASC = CastChecked<ULetheAbilitySystemComponent>(AbilitySystemComponent);
	ASC->AddCharacterAbilities(InCards);
}

void UGASManagerComponent::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const
{
	check(IsValid(AbilitySystemComponent));
	check(GameplayEffectClass);

	const FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
}

void UGASManagerComponent::OnPhaseStateChanged(const EPhaseState OldPhase, const EPhaseState NewPhase) const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (OldPhase == EPhaseState::PlayerTurnPhase)
	{
		AbilitySystemComponent->SetLooseGameplayTagCount(LetheGameplayTags.State_Phase_PlayerTurn, 0);
	}
	
	if (NewPhase == EPhaseState::DrawPhase)
	{
		ApplyRecoveryEffect();
	}
	else if (NewPhase == EPhaseState::PlayerTurnPhase)
	{
		AbilitySystemComponent->AddLooseGameplayTag(LetheGameplayTags.State_Phase_PlayerTurn);
		AbilitySystemComponent->SetLooseGameplayTagCount(LetheGameplayTags.State_Character_MoveConsumed, 0);
	}
}

void UGASManagerComponent::ApplyRecoveryEffect() const
{
	if (!TurnStartRecovery)
	{
		return;
	}

	const FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(TurnStartRecovery, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		SpecHandle.Data->SetSetByCallerMagnitude(LetheGameplayTags.Attributes_Vital_ManaRecovery, AbilitySystemComponent->GetNumericAttribute(ULetheAttributeSet::GetManaRecoveryAttribute()));
		SpecHandle.Data->SetSetByCallerMagnitude(LetheGameplayTags.Attributes_Vital_CostRecovery, AbilitySystemComponent->GetNumericAttribute(ULetheAttributeSet::GetCostRecoveryAttribute()));
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
	}
}
