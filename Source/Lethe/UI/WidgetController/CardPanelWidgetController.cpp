// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/Game/LetheGameState.h"

void UCardPanelWidgetController::BindCallbacksToDependencies(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS)
{
	// Ability가 부여되면 콜백을 받아 해당하는 Card를 생성할 수 있도록 바인드합니다.
	ASC->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);

	// 해당 함수는 캐릭터 수만큼, 최대 4번 호출되기 때문에 플래그로 1번만 콜백이 바인드되도록 막아줍니다.
	if (!bInitialized)
	{
		LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
		if (LetheGameState.IsValid())
		{
			LetheGameState->OnChangePlayerTurnStateDelegate.AddUObject(this, &ThisClass::OnPlayerPhaseChanged);
		}
		
		bInitialized = true;
	}
}

void UCardPanelWidgetController::BeginDestroy()
{
	Super::BeginDestroy();

	if (LetheGameState.IsValid())
	{
		LetheGameState->OnChangePlayerTurnStateDelegate.RemoveAll(this);
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
	if (LetheGameState.IsValid())
	{
		LetheGameState->GoDrawPhase();
	}
}

void UCardPanelWidgetController::GoBattlePhase() const
{
	if (LetheGameState.IsValid())
	{
		LetheGameState->GoBattlePhase();
	}
}

void UCardPanelWidgetController::RequestTurnEnd()
{
	// TODO: 배틀 페이즈가 종료되고 GameState에서 적 턴이 시작되도록 제어해야 합니다.
	// 현재는 단순히 DrawPhase로 돌아가도록 합니다.
	GoDrawPhase();
}

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, const UCardDefinitionData* CardDefinitionData, const UCardSelfViewData* CardSelfViewData, const UCharacterDefinitionData* CharacterDefinitionData) const
{
	FCardInitParams InitParams;
	InitParams.OwnerASC = OwnerASC;
	InitParams.CardViewData = CardViewData;
	InitParams.CardDefinition = CardDefinitionData;
	InitParams.CardSelfViewData = CardSelfViewData;
	InitParams.CharacterDefinitionData = CharacterDefinitionData;
	OnAbilityUpdatedDelegate.ExecuteIfBound(InitParams);
}

void UCardPanelWidgetController::OnPlayerPhaseChanged(const EPlayerPhaseState InState) const
{
	OnPlayerPhaseStateChangedDelegate.Broadcast(InState);
}
