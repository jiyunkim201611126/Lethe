// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidgetController.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/Game/GameState/LetheGameState.h"

void UCardPanelWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams)
{
	Super::SetWidgetControllerParams(WidgetControllerParams);

	// AbilitySystemReferences가 갱신되었음을 알려줍니다.
	OnAbilitySystemReferencesUpdatedDelegate.ExecuteIfBound();
}

void UCardPanelWidgetController::BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS, UPlayerAttributeSet* PAS)
{
	// Ability가 부여되면 콜백을 받아 해당하는 Card를 생성할 수 있도록 바인드합니다.
	ASC->OnAbilityGivenDelegate.BindUObject(this, &ThisClass::OnGiveAbility);

	// 해당 함수는 캐릭터 수만큼, 최대 4번 호출되기 때문에 플래그로 1번만 콜백이 바인드되도록 막아줍니다.
	if (!bInitialized)
	{
		LethePlayerController = CastChecked<ALethePlayerController>(PlayerController);

		LethePlayerController->OnSelectCardDelegate.BindUObject(this, &ThisClass::OnSelectCard);
		LethePlayerController->OnCancelCardSelectCancelDelegate.AddUObject(this, &ThisClass::OnCancelCardSelect);
		LethePlayerController->OnResolveUseCardDelegate.BindUObject(this, &ThisClass::OnResolveUseCard);

		LetheGameState = GetWorld()->GetGameState<ALetheGameState>();
		check(LetheGameState.IsValid());

		bInitialized = true;
	}
}

void UCardPanelWidgetController::OnGiveAbility(const FGrantedCardAbilityInfo& GrantedCardAbilityInfo) const
{
	FCardInitParams InitParams;
	InitParams.OwnerASC = GrantedCardAbilityInfo.OwnerASC;
	InitParams.CharacterDefinitionData = GrantedCardAbilityInfo.CharacterDefinitionData;
	InitParams.CardDefinition = GrantedCardAbilityInfo.CardDefinitionData;
	InitParams.SavedCard = GrantedCardAbilityInfo.SavedCard;
	InitParams.AbilitySpecHandle = GrantedCardAbilityInfo.AbilitySpecHandle;
	InitParams.CardViewData = CardViewData;
	OnAbilityUpdatedDelegate.ExecuteIfBound(InitParams);
}

void UCardPanelWidgetController::BeginDestroy()
{
	Super::BeginDestroy();

	if (LethePlayerController)
	{
		LethePlayerController->OnSelectCardDelegate.Unbind();
		LethePlayerController->OnCancelCardSelectCancelDelegate.RemoveAll(this);
		LethePlayerController->OnResolveUseCardDelegate.Unbind();
	}
}

void UCardPanelWidgetController::GoPlayerTurnPhase() const
{
	if (LetheGameState.IsValid())
	{
		LetheGameState->GoPlayerTurnPhase();
	}
}

bool UCardPanelWidgetController::RequestTurnEnd() const
{
	if (LetheGameState.IsValid())
	{
		// Ability 사용 중이 아닌 상태일 때만 턴을 종료할 수 있습니다.
		if (!LetheGameState->IsResolvingPlayerAbility())
		{
			LetheGameState->GoEnemyTurnPhase();
			return true;
		}
	}
	return false;
}

void UCardPanelWidgetController::UpdateMouseInWorldSection(const bool bIsMouseInWorldSection) const
{
	if (LethePlayerController)
	{
		LethePlayerController->SetMouseOnWorldSection(bIsMouseInWorldSection);
	}
}

void UCardPanelWidgetController::HandleLeftMouseButtonClickedInWorldSection() const
{
	if (LethePlayerController)
	{
		LethePlayerController->HandleLeftMouseButtonClickedInWorldSection();
	}
}

void UCardPanelWidgetController::ResetSelectedCharacter() const
{
	if (LethePlayerController)
	{
		LethePlayerController->ResetSelectedCharacter();
	}
}

void UCardPanelWidgetController::StartResolvePlayerMoves() const
{
	if (LethePlayerController)
	{
		LethePlayerController->StartResolvePlayerMoves();
	}
}

void UCardPanelWidgetController::OnSelectCardRequested(const int32 HandIndex, ULetheAbilitySystemComponent* OwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle) const
{
	if (LethePlayerController)
	{
		LethePlayerController->OnSelectCardRequested(HandIndex, OwnerASC, AbilitySpecHandle);
	}
}

void UCardPanelWidgetController::ResetSelectedCard() const
{
	if (LethePlayerController)
	{
		LethePlayerController->ResetSelectedCard();
	}
}

bool UCardPanelWidgetController::IsCardSelected() const
{
	if (LethePlayerController)
	{
		return LethePlayerController->IsCardSelected();
	}
	return false;
}

void UCardPanelWidgetController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const FGameplayTag& CardTag, const int32 InHandIndex) const
{
	if (LethePlayerController)
	{
		LethePlayerController->RequestUseCard(OwnerASC, AbilitySpecHandle, CardTag, InHandIndex);
	}
}

void UCardPanelWidgetController::OnSelectCard(const int32 HandIndex) const
{
	OnCardSelectedDelegate.ExecuteIfBound(HandIndex);
}

void UCardPanelWidgetController::OnCancelCardSelect() const
{
	OnCardSelectCanceledDelegate.ExecuteIfBound();
}

void UCardPanelWidgetController::OnResolveUseCard(const int32 HandIndex, const bool bSuccess) const
{
	OnUseCardResolvedDelegate.ExecuteIfBound(HandIndex, bSuccess);
}
