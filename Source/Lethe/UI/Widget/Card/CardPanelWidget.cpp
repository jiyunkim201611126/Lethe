// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/Player/LethePlayerController.h"
#include "Lethe/UI/WidgetController/CardPanelWidgetController.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AbilitySystemComponentToCards.Reserve(PLAYABLE_CHARACTER_NUMBER);
	CurrentHands.Reserve(MAX_HAND_COUNT);
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	Super::WidgetControllerSet_Implementation();

	if (UCardPanelWidgetController* CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController))
	{
		AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
		if (!bControllerInitialized)
		{
			// 카드 사이즈에 따라 균일하게 배치될 수 있도록 각종 변수를 조정합니다.
			const FVector2D CardSize = CardPanelWidgetController->GetCardSize();
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
			CardHighlightScale = 1.f / CardPanelWidgetController->GetCardHighlightScale();
			bControllerInitialized = true;
		}
	}

	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(WidgetController->GetPC()))
	{
		PlayerController->OnNumberKeyPressedDelegate.BindUObject(this, &ThisClass::OnKeyboardEvent);
	}
}

void UCardPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CurrentReadyToUseCard.IsValid() && CurrentReadyToUseCard->IsDragging())
	{
		// 드래그 중인 카드가 있을 때만 들어오는 분기입니다.
		if (const APlayerController* PlayerController = GetOwningPlayer())
		{
			FVector2D MousePosition;
			if (PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y))
			{
				// CardPanel의 Geometry를 가져옵니다.
				const FGeometry CardPanelGeometry = GetCachedGeometry();

				// 마우스 스크린 좌표를 CardPanel의 로컬 좌표로 변환합니다.
				FVector2D LocalMousePosition;
				USlateBlueprintLibrary::ScreenToWidgetLocal(GetWorld(), CardPanelGeometry, MousePosition, LocalMousePosition);

				// 앵커가 (0, 1)이므로, Y 위치를 CardPanel의 높이만큼 빼주면 딱 맞습니다.
				const FVector2D CardPanelSize = CardPanelGeometry.GetLocalSize();
				FVector2D FinalTranslation;
				FinalTranslation.X = LocalMousePosition.X;
				FinalTranslation.Y = LocalMousePosition.Y - CardPanelSize.Y;

				// 카드의 중심이 마우스에 있도록 조정합니다.
				CurrentReadyToUseCard->SetRenderTranslation(FinalTranslation);
			}
		}
	}
}

FReply UCardPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (CurrentReadyToUseCard.IsValid())
	{
		// 사용 준비 중인 카드가 있을 때만 들어오는 분기입니다.
		if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
		{
			const bool bUseCardSuccess = PlayerController->RequestUseCard(CurrentReadyToUseCard.Get());
			if (bUseCardSuccess)
			{
				SuccessToUseCard();
			}
			else
			{
				FailToUseCard();
			}
		}
	}
	
	return FReply::Handled();
}

void UCardPanelWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (CurrentReadyToUseCard.IsValid())
	{
		// 카드를 드래그하던 중 마우스 캡쳐를 잃어버린 경우 들어오는 분기입니다.
		FailToUseCard();
	}
	
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
}

void UCardPanelWidget::OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction)
{	
	if (CurrentPlayerPhaseState == EPlayerPhaseState::DrawPhase)
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
	
	if (CurrentPlayerPhaseState == EPlayerPhaseState::BattlePhase)
	{
		// 키보드 입력을 통해 사용 준비 중인 카드가 있었다면, 마우스 입력이 주체가 되었으므로 리셋합니다.
		if (CurrentReadyToUseCard.IsValid() && !CurrentReadyToUseCard->IsDragging())
		{
			CurrentReadyToUseCard->HighlightCard(false);
			CurrentReadyToUseCard.Reset();
		}
		
		switch (InCardAction)
		{
		case ECardAction::HandHovered:
			OnHandHovered(InCardWidget, true);
			break;
		case ECardAction::HandUnhovered:
			OnHandHovered(InCardWidget, false);
			break;
		case ECardAction::ReadyToUse:
			StartDrag(InCardWidget);
			break;
		default:
			break;
		}
	}
}

void UCardPanelWidget::OnKeyboardEvent(const int32 InNumber)
{
	switch (CurrentPlayerPhaseState)
	{
	case EPlayerPhaseState::DrawPhase:
		if (AbilitySystemReferences->IsValidIndex(InNumber))
		{
			if (const FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find((*AbilitySystemReferences)[InNumber].AbilitySystemComponent))
			{
				if (!CharacterCards->Deck.IsEmpty())
				{
					Draw(CharacterCards->Deck[0]);
					UpdateAllCardTranslation();
				}
			}
		}
		break;
	case EPlayerPhaseState::BattlePhase:
		if (CurrentHands.IsValidIndex(InNumber) && CurrentHands[InNumber])
		{
			for (UCardWidget* Hand : CurrentHands)
			{
				Hand->HighlightCard(false);
			}
			
			CurrentHands[InNumber]->HighlightCard(true);
			CurrentReadyToUseCard = CurrentHands[InNumber];
		}
		break;
	default:
		break;
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
		CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);
		
		// Card의 위치, 회전, 크기를 다루기 위한 값들을 설정합니다.
		CreatedCard->SetSize(CardInitParams.CardViewData->GetCardSize() / CardHighlightScale);
		CreatedCard->SetRenderScale(FVector2D(CardHighlightScale));
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
	for (const FAbilitySystemReference& AbilitySystemReference : *AbilitySystemReferences)
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
				FWidgetTransform WidgetTransform = CardInHand->GetRenderTransform();
				WidgetTransform.Translation = NextCardTranslation;
				CardInHand->SetTargetTransform(WidgetTransform);
			
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
				// 마우스 Hovered 여부에 따라 카드를 Highlight합니다.
				DeckOnTopCard->HighlightCard(bInHovered);
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
		if (const UCardPanelWidgetController* CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController))
		{
			CardPanelWidgetController->GoBattlePhase();
		}

		// 마우스를 덱에 올려둔 채로 키보드로 드로우할 경우 DeckHighlight가 남아있는 현상을 해결하기 위해 작성된 구문입니다.
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
	if (bInHovered && CurrentReadyToUseCard.IsValid())
	{
		// 마우스가 핸드 위로 올라왔을 때, 현재 드래그 중인 카드가 있다면 아무런 상호작용을 하지 않습니다.
		return;
	}
	
	// 마우스 Hovered 여부에 따라 카드를 Highlight합니다.
	InCardWidget->HighlightCard(bInHovered);
}

void UCardPanelWidget::StartDrag(UCardWidget* InCardWidget)
{
	CurrentReadyToUseCard = InCardWidget;
	if (CurrentReadyToUseCard.IsValid())
	{
		CurrentReadyToUseCard->SetCardContainer(ECardContainer::Dragging);
		if (UCanvasPanelSlot* DraggingCardSlot = Cast<UCanvasPanelSlot>(CurrentReadyToUseCard->Slot))
		{
			DraggingCardSlot->SetZOrder(DraggingZOrder);
		}
		
		if (ALethePlayerController* PlayerController = GetOwningPlayer<ALethePlayerController>())
		{
			PlayerController->SetReadyToUseCard(true);
		}
	}
}

void UCardPanelWidget::SuccessToUseCard()
{
	// 사용에 성공했으므로 해당하는 Hands 배열에서 제거하고 무덤에 추가합니다.
	if (FCharacterCards* CharacterCards = AbilitySystemComponentToCards.Find(CurrentReadyToUseCard->GetOwnerASC()))
	{
		CharacterCards->Hands.Remove(CurrentReadyToUseCard.Get());
		CharacterCards->Graves.Emplace(CurrentReadyToUseCard.Get());
	}

	// 무덤으로 보낸 뒤 재정렬합니다.
	FWidgetTransform WidgetTransform = CurrentReadyToUseCard->GetRenderTransform();
	WidgetTransform.Translation = GravesCardTranslation;
	CurrentReadyToUseCard->SetTargetTransform(WidgetTransform);
	CurrentReadyToUseCard->SetCardContainer(ECardContainer::Grave);
	CurrentReadyToUseCard.Reset();
	UpdateAllCardTranslation();
}

void UCardPanelWidget::FailToUseCard()
{
	// 사용에 실패했으므로 제자리로 되돌립니다.
	CurrentReadyToUseCard->SetCardContainer(ECardContainer::Hand, true);
	CurrentReadyToUseCard.Reset();
	UpdateAllCardTranslation();
}

void UCardPanelWidget::OnPlayerPhaseStateChanged(const EPlayerPhaseState InState)
{
	CurrentPlayerPhaseState = InState;
}
