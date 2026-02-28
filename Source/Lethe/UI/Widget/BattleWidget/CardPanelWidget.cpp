// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardPanelWidgetController.h"
#include "CardUseSectionWidget.h"
#include "CardWidget.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Lethe/Lethe.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	UseRequestedCards.Reserve(MAX_HAND_COUNT);
	
	TurnEndButton->OnClicked.AddDynamic(this, &ThisClass::OnTurnEndButtonClicked);
	CardUseSection->OnMouseButtonDown.BindUObject(this, &ThisClass::OnMouseButtonDownInCardUseSection);
	CardUseSection->OnMouseButtonUp.BindUObject(this, &ThisClass::OnMouseButtonUpInCardUseSection);
}

void UCardPanelWidget::NativeDestruct()
{
	CardLayoutManager = nullptr;
	TurnEndButton->OnClicked.RemoveDynamic(this, &ThisClass::OnTurnEndButtonClicked);
	
	Super::NativeDestruct();
}

FReply UCardPanelWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ResetSelectedCard();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	if (!CardPanelWidgetController)
	{
		CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController);
	}
	
	// 해당 함수는 캐릭터 수만큼, 최대 4번 호출되기 때문에 플래그로 1번만 아래 로직이 실행되도록 막아줍니다.
	if (!bControllerInitialized && CardPanelWidgetController)
	{
		CardLayoutManager = NewObject<UCardLayoutManager>(this);
		if (CardLayoutManager)
		{			
			// 아웃라인 구현을 위해 카드 사이즈를 4 높게 잡았으므로 그걸 뺀 수치를 사용합니다.
			const FVector2D CardSize = CardPanelWidgetController->GetCardSize() - FVector2D(4.f);
			CardLayoutManager->Initialize(CardSize);
		}
		
		CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
		CardPanelWidgetController->OnPlayerPhaseStateChangedDelegate.AddUObject(this, &ThisClass::OnPlayerPhaseStateChanged);
		CardPanelWidgetController->OnNumberKeyPressedDelegate.BindUObject(this, &ThisClass::OnKeyboardEvent);
		CardPanelWidgetController->OnCancelCardSelectDelegate.BindUObject(this, &ThisClass::ResetSelectedCardWithoutEvent);
		CardPanelWidgetController->OnUseCardResolvedDelegate.BindUObject(this, &ThisClass::OnUseCardResolved);
		
		CardPanelWidgetController->BroadcastInitialValue();
		bControllerInitialized = true;
	}
}

void UCardPanelWidget::OnMouseEvent(UCardWidget* CardWidget, const ECardAction CardAction)
{	
	switch (CurrentPhaseState)
	{
	case EPhaseState::DrawPhase:
		OnMouseEventWhenDrawPhase(CardWidget, CardAction);
		break;
	case EPhaseState::PlayerTurnPhase:
		OnMouseEventWhenPlayerTurnPhase(CardWidget, CardAction);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnMouseEventWhenDrawPhase(const UCardWidget* CardWidget, const ECardAction CardAction)
{
	switch (CardAction)
	{
	case ECardAction::DeckHovered:
		OnDeckHovered(CardWidget, true);
		break;
	case ECardAction::DeckUnhovered:
		OnDeckHovered(CardWidget, false);
		break;
	case ECardAction::Draw:
		TryDraw(CardWidget ? CardWidget->GetOwnerASC() : nullptr);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnMouseEventWhenPlayerTurnPhase(UCardWidget* CardWidget, const ECardAction CardAction)
{
	switch (CardAction)
	{
	case ECardAction::HandHovered:
		OnHandHovered(CardWidget, true);
		break;
	case ECardAction::HandUnhovered:
		OnHandHovered(CardWidget, false);
		break;
	case ECardAction::Selected:
		SelectCard(CardWidget);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnKeyboardEvent(const int32 Number)
{
	switch (CurrentPhaseState)
	{
	case EPhaseState::DrawPhase:
		OnKeyboardEventWhenDrawPhase(Number);
		break;
	case EPhaseState::PlayerTurnPhase:
		OnKeyboardEventWhenPlayerTurnPhase(Number);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnKeyboardEventWhenDrawPhase(const int32 Number) const
{
	if (!CardLayoutManager || !CardPanelWidgetController)
	{
		return;
	}

	const TArray<FAbilitySystemReference>& AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
	if (AbilitySystemReferences.IsValidIndex(Number))
	{
		TryDraw(AbilitySystemReferences[Number].AbilitySystemComponent);
	}
}

void UCardPanelWidget::OnKeyboardEventWhenPlayerTurnPhase(const int32 Number)
{
	if (!CardLayoutManager)
	{
		return;
	}

	const TArray<TObjectPtr<UCardWidget>>& CurrentHands = CardLayoutManager->GetCurrentHands();
	if (CurrentHands.IsValidIndex(Number))
	{
		ResetSelectedCard();
		UCardWidget* SelectedCard = CurrentHands[Number];
		if (SelectedCard->GetCurrentCardContainer() == ECardContainer::Hand)
		{
			SelectCard(SelectedCard);
		}
	}
}

void UCardPanelWidget::CreateCard(const FCardInitParams& CardInitParams)
{
	if (UCardWidget* CreatedCard = CreateWidget<UCardWidget>(this, CardWidgetClass))
	{		
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			CreatedCard->SetWidgetController(WidgetController);
			CreatedCard->SetCardInfo(CardInitParams);
			CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnMouseEvent);

			if (CardLayoutManager)
			{
				CardLayoutManager->SetupCardSlot(CardSlot);
				CardLayoutManager->AddCardToDeck(CreatedCard);
				
				if (CardLayoutManager->AreAllDecksFull())
				{
					CardLayoutManager->ShuffleDeck();
					UpdateAllCardTranslation();
				}
			}
		}
	}
}

void UCardPanelWidget::UpdateAllCardTranslation() const
{
	if (CardLayoutManager && CardPanelWidgetController)
	{
		CardLayoutManager->MoveAllCards(CardPanelWidgetController->GetAbilitySystemReferences());
	}
}

void UCardPanelWidget::OnDeckHovered(const UCardWidget* CardWidget, const bool bHovered) const
{
	if (!CardLayoutManager || !CardWidget)
	{
		return;
	}

	if (ULetheAbilitySystemComponent* OwnerASC = CardWidget->GetOwnerASC())
	{
		if (UCardWidget* DeckOnTopCard = CardLayoutManager->GetTopDeckCard(OwnerASC))
		{
			DeckOnTopCard->MouseHovered(bHovered);
		}
	}
}

void UCardPanelWidget::TryDraw(ULetheAbilitySystemComponent* OwnerASC) const
{
	if (!CardLayoutManager || !CardPanelWidgetController || !OwnerASC)
	{
		return;
	}

	if (CardLayoutManager->TryDraw(OwnerASC))
	{
		UpdateAllCardTranslation();
	}

	if (CardLayoutManager->GetCurrentHandsNum() == MAX_HAND_COUNT)
	{
		// 8장 드로우를 마쳤으므로, 배틀 페이즈에 돌입합니다.
		CardPanelWidgetController->GoPlayerTurnPhase();

		// 마우스를 덱에 올려둔 채로 키보드로 드로우할 경우 DeckHovered가 남아있는 현상을 해결하기 위해 작성된 구문입니다.
		const TArray<FAbilitySystemReference>& AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
		for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
		{
			if (UCardWidget* DeckOnTopCard = CardLayoutManager->GetTopDeckCard(AbilitySystemReference.AbilitySystemComponent))
			{
				DeckOnTopCard->MouseHovered(false);
			}
		}
	}
}

void UCardPanelWidget::OnHandHovered(UCardWidget* CardWidget, const bool bHovered) const
{
	CardWidget->MouseHovered(bHovered);
}

void UCardPanelWidget::SelectCard(UCardWidget* CardWidget)
{
	// 다른 카드가 선택되어 있었다면 선택을 취소합니다.
	ResetSelectedCard();

	// 이미 사용 대기 상태인 카드라면 선택하지 않고 얼리 리턴합니다.
	const int32 HandIndex = CardLayoutManager ? CardLayoutManager->FindCurrentHandIndex(CardWidget) : INDEX_NONE;
	if (UseRequestedCards.Contains(HandIndex))
	{
		return;
	}

	// 이미 사용했거나, 선택된 카드라면 얼리 리턴합니다.
	if (CardWidget->GetCurrentCardContainer() == ECardContainer::Grave || CardWidget->GetCurrentCardContainer() == ECardContainer::Selected)
	{
		return;
	}
	
	CurrentSelectedCard = CardWidget;
	if (CurrentSelectedCard)
	{
		if (CardPanelWidgetController)
		{
			if (CardPanelWidgetController->SetCardSelected(true, CurrentSelectedCard->GetOwnerASC(), CurrentSelectedCard->GetCardTag()))
			{
				CurrentSelectedCard->SetCardContainer(ECardContainer::Selected);
				if (CardLayoutManager)
				{
					CardLayoutManager->OnCardSelected(CurrentSelectedCard);
				}
			}
		}
	}
}

bool UCardPanelWidget::OnMouseButtonDownInCardUseSection() const
{
	return CurrentSelectedCard != nullptr;
}

bool UCardPanelWidget::OnMouseButtonUpInCardUseSection()
{
	if (CurrentSelectedCard && CardPanelWidgetController)
	{
		// 사용 준비 중인 카드가 있을 때만 들어오는 분기입니다.
		const int32 HandIndex = CardLayoutManager ? CardLayoutManager->FindCurrentHandIndex(CurrentSelectedCard) : INDEX_NONE;
		if (HandIndex == INDEX_NONE)
		{
			ResetSelectedCard();
			return false;
		}
		UseRequestedCards.Emplace(HandIndex, CurrentSelectedCard);
		CardPanelWidgetController->RequestUseCard(CurrentSelectedCard->GetOwnerASC(), CurrentSelectedCard->GetCardTag(), HandIndex);

		// 성공 여부와 관계 없이 카드 선택을 취소합니다.
		if (CurrentSelectedCard)
		{
			CurrentSelectedCard->MouseHovered(false);
			CurrentSelectedCard = nullptr;
			
			if (CardPanelWidgetController)
			{
				CardPanelWidgetController->SetCardSelected(false);
			}
		}
		return true;
	}
	return false;
}

void UCardPanelWidget::ResetSelectedCard()
{
	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->SetCardContainer(ECardContainer::Hand, true);
		CurrentSelectedCard = nullptr;

		if (CardPanelWidgetController)
		{
			CardPanelWidgetController->SetCardSelected(false);
		}

		UpdateAllCardTranslation();
	}
}

void UCardPanelWidget::ResetSelectedCardWithoutEvent()
{
	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->SetCardContainer(ECardContainer::Hand, true);
		CurrentSelectedCard = nullptr;

		UpdateAllCardTranslation();
	}
}

void UCardPanelWidget::OnUseCardResolved(const int32 HandIndex, const bool bSuccess)
{
	// 카드 사용 요청 성공 후, Ability 실제 발동 여부와 상관 없이 이 로직으로 들어옵니다.
	// 해당 함수 호출 횟수는 반드시 카드 사용 요청 성공 횟수와 1:1 대응해야 합니다.
	UCardWidget* CardWidget = UseRequestedCards.FindRef(HandIndex);
	if (!CardWidget)
	{
		ensureAlwaysMsgf(false, TEXT("이곳에 절대로 들어와선 안 됩니다. 김지윤한테 문의 바랍니다."));
		return;
	}
	
	if (bSuccess)
	{
		// 사용에 성공했으므로 카드 상태와 위치를 Grave로 갱신합니다.
		if (CardLayoutManager)
		{
			CardLayoutManager->AddCardToGrave(CardWidget);
		}
	}
	else
	{
		// 사용에 실패했으므로 Container 상태를 Selected에서 Hand로 되돌립니다.
		CardWidget->SetCardContainer(ECardContainer::Hand, true);
	}

	// 성공 여부와 관계 없이 사용을 요청했던 카드는 사용 대기 상태를 해제합니다.
	UseRequestedCards.Remove(HandIndex);
}

void UCardPanelWidget::OnTurnEndButtonClicked()
{
	if (CurrentPhaseState == EPhaseState::PlayerTurnPhase)
	{
		const bool bRequestResult = CardPanelWidgetController->RequestTurnEnd();

		if (bRequestResult)
		{
			if (CardLayoutManager)
			{
				CardLayoutManager->AddAllHandsToGrave();
			}

			UpdateAllCardTranslation();
		}
	}
}

void UCardPanelWidget::OnPlayerPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState)
{
	CurrentPhaseState = NewState;

	switch (CurrentPhaseState)
	{
	case EPhaseState::DrawPhase:
		OnDrawPhaseStarted();
		break;
	case EPhaseState::PlayerTurnPhase:
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnDrawPhaseStarted() const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->SetCardSelected(false);
	}
	
	if (CardLayoutManager && CardLayoutManager->AreAllDecksEmpty())
	{
		CardLayoutManager->RefillDeck();
		CardLayoutManager->ShuffleDeck();
		UpdateAllCardTranslation();
	}
}
