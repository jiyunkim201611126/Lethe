// Copyright JETBLU, Inc. All Rights Reserved.

#include "GASManagerComponent.h"

#include "Kismet/GameplayStatics.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"
#include "Lethe/UI/Framework/Policy/BattleUIFeature.h"

UGASManagerComponent::UGASManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetTeamSide() == ETeamSide::Player)
	{
		if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			LetheGameState->OnChangePhaseState.RemoveAll(this);
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

void UGASManagerComponent::SetPlayerAttributeSet(UPlayerAttributeSet* InPlayerAttributeSet)
{
}

UAbilitySystemComponent* UGASManagerComponent::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void UGASManagerComponent::InitAbilityActorInfo(const TArray<UUserWidget*>& AttributeWidgets)
{
	ALetheCharacterBase* OwnerCharacter = GetOwner<ALetheCharacterBase>();
	AbilitySystemComponent->InitAbilityActorInfo(OwnerCharacter, OwnerCharacter);

	InitUI(AttributeWidgets);
	
	ApplyEffectToSelf(DefaultAttributes, 1.f);
	
	AddCharacterAbilities(StartAbilities);

	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangePhaseState.AddUObject(this, &ThisClass::OnPhaseStateChanged);
	}
}

void UGASManagerComponent::InitUI(const TArray<UUserWidget*>& AttributeWidgets)
{
	if (GetTeamSide() != ETeamSide::Enemy)
	{
		return;
	}
	
	// PlayerController가 빙의하는 캐릭터가 아니기 때문에 라이브러리 함수로 가져옵니다.
	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0);
	const ULetheUIManagerSubsystem* UIManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULetheUIManagerSubsystem>();
	if (!PlayerController || !UIManagerSubsystem)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	UBattleUIFeature* BattleUIFeature = UIManagerSubsystem->FindUIFeatureWithEnsureRootLayout<UBattleUIFeature>(LocalPlayer);
	if (!BattleUIFeature)
	{
		return;
	}
	
	ULetheWidgetController* WidgetController = BattleUIFeature->CreateEnemyAttributeWidgetController(PlayerController, AbilitySystemComponent, AttributeSet);
	for (UUserWidget* AttributeWidget : AttributeWidgets)
	{
		CastChecked<ULetheUserWidget>(AttributeWidget)->SetWidgetController(WidgetController);
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

void UGASManagerComponent::OnDied() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	ULetheAbilitySystemComponent* ASC = CastChecked<ULetheAbilitySystemComponent>(AbilitySystemComponent);
	ASC->AddLooseGameplayTag(LetheGameplayTags.State_Character_Dead, 1);
}

bool UGASManagerComponent::IsDead() const
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	const ULetheAbilitySystemComponent* ASC = CastChecked<ULetheAbilitySystemComponent>(AbilitySystemComponent);
	return ASC->HasMatchingGameplayTag(LetheGameplayTags.State_Character_Dead);
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
	if (NewPhase == EPhaseState::EnemyPlanningPhase)
	{
		OnPlanPhaseStarted();
	}
	
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	AbilitySystemComponent->SetLooseGameplayTagCount(LetheGameplayTags.State_Character_CanAct, 0);
	
	if (NewPhase == EPhaseState::EnemyTurnPhase)
	{
		AbilitySystemComponent->AddLooseGameplayTag(LetheGameplayTags.State_Character_CanAct);
	}
}

void UGASManagerComponent::OnPlanPhaseStarted() const
{
	/**
	 * 모든 ASC에게서 MoveConsumed 태그를 제거합니다.
	 * 예외적으로 GE가 아닌 코드로 직접 수정하는 이유는, GE로 구현한다면 태그 제거가 깔끔하지 않기 때문입니다.
	 * MoveConsumed를 제외한 모든 '행동 제한'형 태그는 전부 GE의 'Grant Tags To Target Actor'로 부여하고, Ability에서 'ActivationBlockedTags'로 걸러야 합니다.
	 */
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	AbilitySystemComponent->SetLooseGameplayTagCount(LetheGameplayTags.State_Character_MoveConsumed, 0);
}

ETeamSide UGASManagerComponent::GetTeamSide() const
{
	if (const ICombatInterface* CombatInterface = GetOwner<ICombatInterface>())
	{
		return CombatInterface->GetTeamSide();
	}
	return ETeamSide::None;
}
