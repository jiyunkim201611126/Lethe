// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/Game/LetheGameState.h"

void UCardPanelWidgetController::BindCallbacksToDependencies(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	// Ability가 부여되면 콜백을 받아 해당하는 Card를 생성할 수 있도록 바인드합니다.
	ASC->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);

	// 해당 함수는 캐릭터 수만큼, 최대 4번 호출되기 때문에 플래그로 1번만 콜백이 바인드되도록 막아줍니다.
	if (!bInitialized)
	{
		if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
		{
			LetheGameState->OnChangePlayerTurnStateDelegate.AddUObject(this, &ThisClass::OnPlayerPhaseChanged);
		}
		
		bInitialized = true;
	}
}

void UCardPanelWidgetController::BroadcastInitialValue()
{
	GoDrawPhase();
}

FVector2D UCardPanelWidgetController::GetCardSize() const
{
	return CardViewData->GetCardSize();
}

float UCardPanelWidgetController::GetCardHighlightScale() const
{
	return CardViewData->GetCardHighlightScale();
}

void UCardPanelWidgetController::GoDrawPhase() const
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->GoDrawPhase();
	}
}

void UCardPanelWidgetController::GoBattlePhase() const
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->GoBattlePhase();
	}
}

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const UCardDefinitionData* CardDefinitionData, const UCardSelfViewData* CardSelfViewData, const UCardOwnerViewData* CardOwnerViewData) const
{
	FCardInitParams InitParams;
	InitParams.OwnerASC = OwnerASC;
	InitParams.CardViewData = CardViewData;
	InitParams.CardDefinition = CardDefinitionData;
	InitParams.CardSelfViewData = CardSelfViewData;
	InitParams.CardOwnerViewData = CardOwnerViewData;
	OnAbilityUpdatedDelegate.ExecuteIfBound(InitParams);
}

void UCardPanelWidgetController::OnPlayerPhaseChanged(const EPlayerPhaseState InState) const
{
	OnPlayerPhaseStateChangedDelegate.Broadcast(InState);
}
