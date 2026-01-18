// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"

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

void UCardPanelWidgetController::OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, ULetheGameplayAbility* InAbility) const
{
	// Ability에서 CardDescription을 가져와 DataAsset에 넣어줍니다.
	FCardSelfViewInfo* CardSelfViewInfo = CardViewData->FindCardSelfViewInfoByTag(InAbility->CardTag);
	if (CardSelfViewInfo->CardDescriptionText.IsEmpty())
	{
		const FText CardDescriptionText = InAbility->GetCardDescription(InAbility->GetAbilityLevel());
		CardSelfViewInfo->CardDescriptionText = CardDescriptionText;
	}

	// 콜백을 받은 CardPanelWidget이 필요한 데이터를 참조할 수 있도록 보내줍니다.
	if (IPlayableCharacterInterface* OwnerCharacter = Cast<IPlayableCharacterInterface>(OwnerASC->GetOwner()))
	{
		const FCardInitParams InitParams(OwnerASC, CardViewData, InAbility->CardTag, OwnerCharacter->GetCharacterTag());
		OnAbilityUpdatedDelegate.ExecuteIfBound(InitParams);
	}
}

void UCardPanelWidgetController::OnPlayerPhaseChanged(const EPlayerPhaseState InState) const
{
	OnPlayerPhaseStateChangedDelegate.Broadcast(InState);
}
