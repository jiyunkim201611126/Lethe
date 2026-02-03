// Copyright JETBLU, Inc. All Rights Reserved.

#include "GASManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/DeckManagerSubsystem.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"
#include "Lethe/UI/HUD/LetheHUD.h"

UGASManagerComponent::UGASManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
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
	ApplyEffectToSelf(DefaultAttributes, 1.f);

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
					LetheHUD->InitEnemyUI(AbilitySystemComponent, AttributeSet, AttributeWidget);
					break;
				}
			}
		}
	}


	// UDeckManagerSubsystem에서 Owner의 EquippedDeck을 가져옵니다.
	if (IPlayableCharacterInterface* OwnerCharacter = Cast<IPlayableCharacterInterface>(OwnerPawn))
	{
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
