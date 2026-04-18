// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/Game/GameState/LetheGameState.h"

void UCardPanelWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	// Ability가 부여되면 콜백을 받아 해당하는 Card를 생성할 수 있도록 바인드합니다.
	ASC->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);

	// 해당 함수는 캐릭터 수만큼, 최대 4번 호출되기 때문에 플래그로 1번만 콜백이 바인드되도록 막아줍니다.
	if (!bInitialized)
	{
		LethePlayerController = CastChecked<ALethePlayerController>(PlayerController);
		
		LethePlayerController->OnNumberKeyPressedDelegate.BindUObject(this, &ThisClass::OnNumberKeyPressed);
		LethePlayerController->OnCancelCardSelectCancelDelegate.AddUObject(this, &ThisClass::OnCancelCardSelect);
		LethePlayerController->OnResolveUseCardDelegate.BindUObject(this, &ThisClass::OnResolveUseCard);
		
		LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
		check(LetheGameState.IsValid());
		
		LetheGameState->OnChangePhaseState.AddUObject(this, &ThisClass::OnPhaseStateChanged);
		
		bInitialized = true;
	}
}

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const UCardDefinitionData* CardDefinitionData, const UCharacterDefinitionData* CharacterDefinitionData) const
{
	FCardInitParams InitParams;
	InitParams.OwnerASC = OwnerASC;
	InitParams.CardViewData = CardViewData;
	InitParams.CardDefinition = CardDefinitionData;
	InitParams.CharacterDefinitionData = CharacterDefinitionData;
	OnAbilityUpdatedDelegate.ExecuteIfBound(InitParams);
}

void UCardPanelWidgetController::BeginDestroy()
{
	Super::BeginDestroy();

	if (LethePlayerController)
	{
		LethePlayerController->OnResolveUseCardDelegate.Unbind();
	}

	if (LetheGameState.IsValid())
	{
		LetheGameState->OnChangePhaseState.RemoveAll(this);
	}
}

FVector2D UCardPanelWidgetController::GetCardSize() const
{
	return CardViewData->GetCardSize();
}

bool UCardPanelWidgetController::SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag) const
{
	if (LethePlayerController)
	{
		return LethePlayerController->SetCardSelected(bInCardSelected, OwnerASC, CardTag);
	}
	return false;
}

void UCardPanelWidgetController::GoPlayerTurnPhase() const
{
	if (LetheGameState.IsValid())
	{
		LetheGameState->GoPlayerTurnPhase();
	}
}

void UCardPanelWidgetController::StartResolvePlayerMoves() const
{
	if (LethePlayerController)
	{
		LethePlayerController->StartResolvePlayerMoves();
	}
}

bool UCardPanelWidgetController::RequestTurnEnd() const
{
	if (LetheGameState.IsValid() && LethePlayerController)
	{
		// Ability 사용 중이 아닌 상태일 때만 턴을 종료할 수 있습니다.
		if (!LetheGameState->IsResolvingPlayerAbility())
		{
			LethePlayerController->SetCardSelected(false);
			LethePlayerController->ResetSelectedCharacter();
			LetheGameState->GoEnemyTurnPhase();
			return true;
		}
	}
	return false;
}

void UCardPanelWidgetController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex) const
{
	if (LethePlayerController)
	{
		LethePlayerController->RequestUseCard(OwnerASC, CardTag, InHandIndex);
	}
}

void UCardPanelWidgetController::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const
{
	if (LethePlayerController)
	{
		LethePlayerController->GetCardDescriptionText(OwnerASC, CardTag, OutText);
	}
}

void UCardPanelWidgetController::OnPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState) const
{
	OnPhaseStateChangedDelegate.Broadcast(OldState, NewState);
}

void UCardPanelWidgetController::OnNumberKeyPressed(const int32 InNumber) const
{
	OnNumberKeyPressedDelegate.ExecuteIfBound(InNumber);
}

void UCardPanelWidgetController::OnCancelCardSelect() const
{
	OnCardSelectCanceledDelegate.ExecuteIfBound();
}

void UCardPanelWidgetController::OnResolveUseCard(const int32 HandIndex, const bool bSuccess) const
{
	OnUseCardResolvedDelegate.ExecuteIfBound(HandIndex, bSuccess);
}
