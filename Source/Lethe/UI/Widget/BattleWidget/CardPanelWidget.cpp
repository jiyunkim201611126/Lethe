// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardUseSectionWidget.h"
#include "CardWidget.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"
#include "Lethe/UI/WidgetController/CardPanelWidgetController.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AbilitySystemComponentToCards.Reserve(PLAYABLE_CHARACTER_NUMBER);
	CurrentHands.Reserve(MAX_HAND_COUNT);
	
	TurnEndButton->OnClicked.AddDynamic(this, &ThisClass::OnTurnEndButtonClicked);
	CardUseSection->OnMouseButtonUp.BindUObject(this, &ThisClass::TryUseCard);
}

void UCardPanelWidget::NativeDestruct()
{
	Super::NativeDestruct();

	TurnEndButton->OnClicked.RemoveDynamic(this, &ThisClass::OnTurnEndButtonClicked);
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	Super::WidgetControllerSet_Implementation();

	CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController);
	if (CardPanelWidgetController)
	{
		if (!bControllerInitialized)
		{
			// 카드 사이즈에 따라 균일하게 배치될 수 있도록 각종 변수를 조정합니다.
			// 아웃라인 구현을 위해 카드 사이즈를 4 높게 잡았으므로 그걸 뺀 수치를 사용합니다.
			const FVector2D CardSize = CardPanelWidgetController->GetCardSize() - FVector2D(4.f);
			PaddingDeckAndHand += CardSize.X;
			PaddingHandAndHand += CardSize.X;
			FirstCardTranslation.X += CardSize.X / 2.f;
			FirstCardTranslation.Y -= CardSize.Y / 2.f;
			NextCardTranslation.X += CardSize.X / 2.f;
			NextCardTranslation.Y -= CardSize.Y / 2.f;
			GravesCardTranslation.X += CardSize.X / 2.f;
			GravesCardTranslation.Y -= CardSize.Y / 2.f;
			
			CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
			CardPanelWidgetController->OnPlayerPhaseStateChangedDelegate.AddUObject(this, &ThisClass::OnPlayerPhaseStateChanged);
			
			CardPanelWidgetController->BroadcastInitialValue();
			
			// 카드 크기 조정이 필요할 때, RenderScale을 1.f 이상 수치로 사용하면 텍스쳐가 깨져버립니다.
			// 그렇다고 CanvasPanelSlot을 사용하면 CanvasPanel이 CPU한테 염병을 떨기 때문에, Slot은 최대한 건드리지 않는 게 좋습니다.
			// 따라서 기본 사이즈를 1.f 미만 수치로 사용하고, 확대가 필요할 때 1.f로 설정합니다.
			CardExpandScale = 1.f / CardPanelWidgetController->GetCardExpandScale();
			bControllerInitialized = true;
		}
	}

	// 키보드 입력에 반응할 수 있도록 콜백을 바인드합니다.
	if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
	{
		PlayerController->OnNumberKeyPressedDelegate.BindUObject(this, &ThisClass::OnKeyboardEvent);
	}
}

void UCardPanelWidget::OnMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction)
{	
	switch (CurrentPlayerPhaseState)
	{
	case EPlayerPhaseState::DrawPhase:
		OnMouseEventWhenDrawPhase(InCardWidget, InCardAction);
		break;
	case EPlayerPhaseState::BattlePhase:
		OnMouseEventWhenBattlePhase(InCardWidget, InCardAction);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnMouseEventWhenDrawPhase(const UCardWidget* InCardWidget, const ECardAction InCardAction)
{
	switch (InCardAction)
	{
	case ECardAction::DeckHovered:
		OnDeckHovered(InCardWidget, true);
		break;
	case ECardAction::DeckUnhovered:
		OnDeckHovered(InCardWidget, false);
		break;
	case ECardAction::Draw:
		Draw(InCardWidget);
		UpdateAllCardTranslation();
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnMouseEventWhenBattlePhase(UCardWidget* InCardWidget, const ECardAction InCardAction)
{
	switch (InCardAction)
	{
	case ECardAction::HandHovered:
		OnHandHovered(InCardWidget, true);
		break;
	case ECardAction::HandUnhovered:
		OnHandHovered(InCardWidget, false);
		break;
	case ECardAction::Selected:
		SelectCard(InCardWidget);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnKeyboardEvent(const int32 InNumber)
{
	switch (CurrentPlayerPhaseState)
	{
	case EPlayerPhaseState::DrawPhase:
		OnKeyboardEventWhenDrawPhase(InNumber);
		break;
	case EPlayerPhaseState::BattlePhase:
		OnKeyboardEventWhenBattlePhase(InNumber);
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnKeyboardEventWhenDrawPhase(const int32 InNumber)
{
	const TArray<FAbilitySystemReference>& AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
	if (AbilitySystemReferences.IsValidIndex(InNumber))
	{
		if (const FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(AbilitySystemReferences[InNumber].AbilitySystemComponent))
		{
			if (!CharacterCards->Deck.IsEmpty())
			{
				Draw(CharacterCards->Deck[0]);
				UpdateAllCardTranslation();
			}
		}
	}
}

void UCardPanelWidget::OnKeyboardEventWhenBattlePhase(const int32 InNumber)
{
	if (CurrentHands.IsValidIndex(InNumber))
	{
		ResetSelectedCard();
		UCardWidget* SelectedCard = CurrentHands[InNumber];
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
		// 만들어진 Card를 OwnerASC와 매핑된 Deck 배열에 추가합니다.			
		FCharacterCards& CharacterCards = AbilitySystemComponentToCards.FindOrAdd(CardInitParams.OwnerASC);
		CharacterCards.Deck.Emplace(CreatedCard);

		CreatedCard->SetWidgetController(WidgetController);
		CreatedCard->SetCardInfo(CardInitParams);
		CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnMouseEvent);
		
		// Card의 위치, 회전, 크기를 다루기 위한 값들을 설정합니다.
		CreatedCard->SetSize(CardInitParams.CardViewData->GetCardSize() / CardExpandScale);
		CreatedCard->SetRenderScale(FVector2D(CardExpandScale));
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			// 앵커를 좌하단에 박습니다.
			CardSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardSlot->SetAutoSize(true);
			// 덱은 완전히 겹쳐있기 때문에 가장 윗장을 제외하면 ZOrder를 굳이 다르게 할 필요가 없습니다.
			// 따라서 가능한 경우 언리얼이 DrawCall을 병합시킬 수 있도록 ZOrder를 같게 설정해 최적화를 챙깁니다.
			CardSlot->SetZOrder(DeckZOrder);
		}

		// 일단 1장 만들어질 때마다 호출하지만,
		// TODO: 추후 덱의 크기가 40장으로 고정되면 40장이 만들어졌을 때, 혹은 게임 시작 시 등 이벤트를 받아 한 번만 호출하도록 변경합니다.
		UpdateAllCardTranslation();
	}
}

void UCardPanelWidget::UpdateAllCardTranslation()
{
	// 변수들을 초기화합니다.
	HandZOrder = 200;
	CurrentHands.Reset();
	
	// ASC를 순서대로 순회합니다.
	const TArray<FAbilitySystemReference>& AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
	for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
	{
		FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(AbilitySystemReference.AbilitySystemComponent);
		if (!CharacterCards)
		{
			continue;
		}

		for (UCardWidget* CardInDeck : CharacterCards->Deck)
		{
			FWidgetTransform WidgetTransform = CardInDeck->GetRenderTransform();
			WidgetTransform.Translation = NextCardTranslation;
			CardInDeck->SetTargetTransform(WidgetTransform);
			
			if (UCanvasPanelSlot* LastDeckCardSlot = Cast<UCanvasPanelSlot>(CardInDeck->Slot))
			{
				LastDeckCardSlot->SetZOrder(DeckZOrder);
			}
		}

		if (!CharacterCards->Deck.IsEmpty())
		{
			if (UCanvasPanelSlot* LastDeckCardSlot = Cast<UCanvasPanelSlot>(CharacterCards->Deck.Last()->Slot))
			{
				// 덱의 가장 윗장은 덱 아랫장을 가릴 수 있도록 ZOrder를 1 높게 설정합니다.
				LastDeckCardSlot->SetZOrder(DeckZOrder + 1);
			}
		}

		NextCardTranslation.X += PaddingDeckAndHand;
		
		for (uint8 HandIndex = 0; HandIndex < CharacterCards->Hands.Num(); ++HandIndex)
		{
			if (UCardWidget* CardInHand = CharacterCards->Hands[HandIndex])
			{
				if (CardInHand->GetCurrentCardContainer() == ECardContainer::Hand)
				{
					// 사용한 카드는 로직상으로는 아직 배열에 남아있지만 View 상으로는 무덤 위치로 가있어야 합니다.
					// 따라서 CurrentCardContainer가 Hand인 경우만 아래 위치 이동 로직을 수행합니다.
					FWidgetTransform WidgetTransform = CardInHand->GetRenderTransform();
					WidgetTransform.Translation = NextCardTranslation;
					CardInHand->SetTargetTransform(WidgetTransform);
				}
			
				if (UCanvasPanelSlot* HandCardSlot = Cast<UCanvasPanelSlot>(CardInHand->Slot))
				{
					HandCardSlot->SetZOrder(HandZOrder++);
				}
				
				// 핸드의 마지막 장이면 DeckAndHand로, 아니라면 HandAndHand로 사이 공간을 띄워줍니다.
				NextCardTranslation.X += HandIndex == CharacterCards->Hands.Num() - 1 ? PaddingDeckAndHand : PaddingHandAndHand;

				// 키보드 이벤트에 대해 효율적으로 반응할 수 있도록 캐싱합니다.
				CurrentHands.Emplace(CardInHand);
			}
		}
	}

	NextCardTranslation = FirstCardTranslation;
}

void UCardPanelWidget::OnDeckHovered(const UCardWidget* InCardWidget, const bool bInHovered)
{
	if (ULetheAbilitySystemComponent* OwnerASC = InCardWidget->GetOwnerASC())
	{
		if (FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(OwnerASC))
		{
			// 덱 가장 윗장을 가져옵니다.
			if (UCardWidget* DeckOnTopCard = CharacterCards->Deck.Last())
			{
				// 마우스 Hovered 여부에 따라 카드를 위로 살짝 올려줍니다.
				DeckOnTopCard->MouseHovered(bInHovered);
			}
		}
	}
}

void UCardPanelWidget::Draw(const UCardWidget* InCardWidget)
{
	if (ULetheAbilitySystemComponent* OwnerASC = InCardWidget->GetOwnerASC())
	{
		if (FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(OwnerASC))
		{
			if (UCardWidget* DrawnCardWidget = CharacterCards->Deck.Pop(EAllowShrinking::No))
			{
				// 카드를 핸드에 추가하고 그에 맞게 정렬될 수 있도록 합니다.
				CharacterCards->Hands.Emplace(DrawnCardWidget);
				DrawnCardWidget->SetCardContainer(ECardContainer::Hand);
				UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(DrawnCardWidget->Slot);
				CardSlot->SetZOrder(HandZOrder++);

				// UpdateAllCardTranslation에 의해 배열은 Reset 후 다시 채워지지만, 드로우 페이즈가 끝났는지 확인하기 위해 임시로 1개 채워줍니다.
				CurrentHands.Emplace(DrawnCardWidget);
			}
		}
	}

	if (CurrentHands.Num() == MAX_HAND_COUNT)
	{
		// 8장 드로우를 마쳤으므로, 배틀 페이즈에 돌입합니다.
		CardPanelWidgetController->GoBattlePhase();

		// 마우스를 덱에 올려둔 채로 키보드로 드로우할 경우 DeckHovered가 남아있는 현상을 해결하기 위해 작성된 구문입니다.
		for (const auto& CharacterCards : AbilitySystemComponentToCards)
		{
			if (!CharacterCards.Value.Deck.IsEmpty())
			{
				OnDeckHovered(CharacterCards.Value.Deck[0], false);
			}
		}
	}
}

void UCardPanelWidget::OnHandHovered(UCardWidget* InCardWidget, const bool bInHovered) const
{	
	// 마우스가 Hovered 상태에 따라 카드 위치를 조정합니다.
	InCardWidget->MouseHovered(bInHovered);
}

void UCardPanelWidget::SelectCard(UCardWidget* InCardWidget)
{
	// 다른 카드가 선택되어 있었다면 선택을 취소합니다.
	ResetSelectedCard();
	
	CurrentSelectedCard = InCardWidget;
	if (CurrentSelectedCard.IsValid())
	{
		CurrentSelectedCard->SetCardContainer(ECardContainer::Selected);
		
		if (UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(CurrentSelectedCard->Slot))
		{
			CardSlot->SetZOrder(SelectedZOrder);
		}
		
		if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
		{
			PlayerController->SetCardSelected(true);
		}
	}
}

void UCardPanelWidget::TryUseCard()
{
	if (CurrentSelectedCard.IsValid())
	{
		// 사용 준비 중인 카드가 있을 때만 들어오는 분기입니다.
		if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
		{
			const bool bUseCardSuccess = PlayerController->RequestUseCard(CurrentSelectedCard->GetOwnerASC(), CurrentSelectedCard->GetCardTag());
			bUseCardSuccess ? UseCardSuccess() : ResetSelectedCard();
		}
	}
}

void UCardPanelWidget::UseCardSuccess()
{
	// 사용에 성공했으므로 해당하는 카드를 무덤 배열에 추가합니다.
	// 비주얼상 핸드의 위치가 남아있어야 하기 때문에 핸드 배열은 그대로 두고, 나중에 턴 종료 시 정리합니다.
	if (FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(CurrentSelectedCard->GetOwnerASC()))
	{
		CharacterCards->Graves.Emplace(CurrentSelectedCard.Get());
	}

	FWidgetTransform WidgetTransform = CurrentSelectedCard->GetRenderTransform();
	WidgetTransform.Translation = GravesCardTranslation;
	CurrentSelectedCard->SetTargetTransform(WidgetTransform);
	CurrentSelectedCard->SetCardContainer(ECardContainer::Grave);
	CurrentSelectedCard->MouseHovered(false);
	CurrentSelectedCard.Reset();
	
	if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
	{
		PlayerController->SetCardSelected(false);
	}
}

void UCardPanelWidget::ResetSelectedCard()
{
	if (CurrentSelectedCard.IsValid())
	{		
		CurrentSelectedCard->SetCardContainer(ECardContainer::Hand, true);
		CurrentSelectedCard->MouseHovered(false);
		CurrentSelectedCard.Reset();
		
		if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
		{
			PlayerController->SetCardSelected(false);
		}

		UpdateAllCardTranslation();
	}
}

void UCardPanelWidget::OnTurnEndButtonClicked()
{
	if (CurrentPlayerPhaseState == EPlayerPhaseState::BattlePhase)
	{
		const bool bRequestResult = CardPanelWidgetController->RequestTurnEnd();

		if (bRequestResult)
		{
			// 성공적으로 턴을 마치면 들어오는 분기입니다.
			const TArray<FAbilitySystemReference>& AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
			for (const FAbilitySystemReference& AbilitySystemReference : AbilitySystemReferences)
			{
				if (FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(AbilitySystemReference.AbilitySystemComponent))
				{
					// 핸드에 남아있던 카드를 묘지로 보내고 완전히 비웁니다.
					for (UCardWidget* CardInHand : CharacterCards->Hands)
					{
						CharacterCards->Graves.AddUnique(CardInHand);

						FWidgetTransform WidgetTransform = CardInHand->GetRenderTransform();
						WidgetTransform.Translation = GravesCardTranslation;
						CardInHand->SetTargetTransform(WidgetTransform);
						CardInHand->SetCardContainer(ECardContainer::Grave);
					}
					CharacterCards->Hands.Reset();
				}
			}

			UpdateAllCardTranslation();
		}
	}
}

void UCardPanelWidget::OnPlayerPhaseStateChanged(const EPlayerPhaseState InState)
{
	CurrentPlayerPhaseState = InState;

	switch (CurrentPlayerPhaseState)
	{
	case EPlayerPhaseState::DrawPhase:
		OnDrawPhaseStarted();
		break;
	case EPlayerPhaseState::BattlePhase:
		break;
	default:
		break;
	}
}

void UCardPanelWidget::OnDrawPhaseStarted() const
{
	if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
	{
		PlayerController->SetCardSelected(false);
	}
}
