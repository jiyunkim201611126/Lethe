// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardWidget.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/CardViewData.h"
#include "Lethe/UI/WidgetController/CardPanelWidgetController.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	AbilitySystemComponentToCards.Reserve(PLAYABLE_CHARACTER_NUMBER);
}

void UCardPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CurrentDraggingCard.IsValid())
	{
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

				CurrentDraggingCard->SetRenderTranslation(FinalTranslation);
			}
		}
	}
}

FReply UCardPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (CurrentDraggingCard.IsValid())
	{
		// TODO: 사용 성공 판별 및 그 여부에 따라 처리하는 로직이 필요합니다.
		// 현재는 무조건 실패하도록 되어 있습니다.
		CurrentDraggingCard->SetCardContainer(ECardContainer::Hand, false);
		CurrentDraggingCard.Reset();
		UpdateAllCardTranslation();
	}
	
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	Super::WidgetControllerSet_Implementation();

	if (UCardPanelWidgetController* CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController))
	{
		AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
		CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
		if (!bControllerInitialized)
		{
			PaddingDeckAndHand += CardPanelWidgetController->GetCardSize().X;
			PaddingHandAndHand += CardPanelWidgetController->GetCardSize().X;
			
			// 카드 크기 조정이 필요할 때, RenderScale을 1.f 이상 수치로 사용하면 텍스쳐가 깨져버립니다.
			// 그렇다고 CanvasPanelSlot을 사용하면 CanvasPanel이 CPU한테 염병을 떨기 때문에, Slot은 최대한 건드리지 않는 게 좋습니다.
			// 따라서 기본 사이즈를 1.f 미만 수치로 사용하고, 확대가 필요할 때 1.f로 설정합니다.
			CardHighlightScale = 1.f / CardPanelWidgetController->GetCardHighlightScale();
			bControllerInitialized = true;
		}
	}
}

void UCardPanelWidget::OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction)
{
	switch (InCardAction)
	{
	case ECardAction::DeckHovered:
		if (CurrentHandsNum != MaxHandsNum)
		{
			OnDeckHovered(InCardWidget, true);
		}
		break;
	case ECardAction::DeckUnhovered:
		if (CurrentHandsNum != MaxHandsNum)
		{
			OnDeckHovered(InCardWidget, false);
		}
		break;
	case ECardAction::Draw:
		if (CurrentHandsNum != MaxHandsNum)
		{
			Draw(InCardWidget);
			UpdateAllCardTranslation();
		}
		break;
	case ECardAction::HandHovered:
		if (CurrentHandsNum == MaxHandsNum)
		{
			OnHandHovered(InCardWidget, true);
		}
		break;
	case ECardAction::HandUnhovered:
		if (CurrentHandsNum == MaxHandsNum)
		{
			OnHandHovered(InCardWidget, false);
		}
		break;
	case ECardAction::Drag:
		if (CurrentHandsNum >= 1)
		{
			StartDrag(InCardWidget);
		}
		break;
	case ECardAction::Use:
		break;
	default:
		break;
	}
}

void UCardPanelWidget::CreateCard(const FCardInitParams& CardInitParams)
{
	if (const FCardViewInfo* CardViewInfo = CardInitParams.CardViewData->FindCardInfoByTag(CardInitParams.CardTag))
	{
		if (UCardWidget* CreatedCard = CreateWidget<UCardWidget>(this, CardWidgetClass))
		{
			// 만들어진 Card를 OwnerASC와 매핑된 Deck 배열에 추가합니다.			
			FCharacterCards& CharacterCards = AbilitySystemComponentToCards.FindOrAdd(CardInitParams.OwnerASC);
			CharacterCards.Deck.Emplace(CreatedCard);

			CreatedCard->SetWidgetController(WidgetController);
			CreatedCard->SetOwnerASC(CardInitParams.OwnerASC);
			CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);

			CreatedCard->SetCardView(CardViewInfo);
			CreatedCard->SetCardColor(CardInitParams.CardFrontsideColor, CardInitParams.CardBacksideColor);
		
			// Card의 위치, 회전, 크기를 다루기 위한 값들을 설정합니다.
			CreatedCard->SetRenderTransformPivot(FVector2D(0.f, 1.f));
			CreatedCard->SetRenderScale(FVector2D(CardHighlightScale));
			if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
			{
				// 앵커를 좌하단에 박습니다.
				CardSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
				CardSlot->SetAlignment(FVector2D(0.f, 1.f));
				CardSlot->SetSize(CardInitParams.CardViewData->GetCardSize() / CardHighlightScale);
				// 가능한 경우 언리얼이 DrawCall을 병합시킬 수 있도록 ZOrder를 같게 설정합니다.
				CardSlot->SetZOrder(DeckZOrder);
			}

			// 일단 1장 만들어질 때마다 호출하지만,
			// TODO: 추후 덱의 크기가 40장으로 고정되면 40장이 만들어졌을 때, 혹은 게임 시작 시 등 이벤트를 받아 한 번만 호출하도록 변경합니다.
			UpdateAllCardTranslation();
		}
	}
}

void UCardPanelWidget::UpdateAllCardTranslation()
{
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
			CardInDeck->SetTargetPivotAndTransform(DefaultPivot, WidgetTransform);
			
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
				CardInHand->SetTargetPivotAndTransform(DefaultPivot, WidgetTransform);
				
				// 핸드의 마지막 장이면 DeckAndHand로, 아니라면 HandAndHand로 사이 공간을 띄워줍니다.
				NextCardTranslation.X += HandIndex == CharacterCards->Hands.Num() - 1 ? PaddingDeckAndHand : PaddingHandAndHand;
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

				++CurrentHandsNum;
			}
		}
	}
}

void UCardPanelWidget::OnHandHovered(UCardWidget* InCardWidget, const bool bInHovered) const
{
	// 마우스 Hovered 여부에 따라 카드를 Highlight합니다.
	InCardWidget->HighlightCard(bInHovered);
}

void UCardPanelWidget::StartDrag(UCardWidget* InCardWidget)
{
	CurrentDraggingCard = InCardWidget;
	if (CurrentDraggingCard.IsValid())
	{
		CurrentDraggingCard->SetCardContainer(ECardContainer::Dragging);
		CurrentDraggingCard->SetPivot(DraggingPivot);
	}
}
