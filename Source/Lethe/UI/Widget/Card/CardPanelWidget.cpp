// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/UI/WidgetController/CardPanelWidgetController.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	check(DeckPositions.Num() == PLAYABLE_CHARACTER_NUMBER);
	AbilitySystemComponentToCards.Reserve(PLAYABLE_CHARACTER_NUMBER);
	Hands.Reserve(12);
	Graves.Reserve(40);
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	Super::WidgetControllerSet_Implementation();

	if (UCardPanelWidgetController* CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController))
	{
		AbilitySystemReferences = CardPanelWidgetController->GetAbilitySystemReferences();
		CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
	}
}

void UCardPanelWidget::CreateCard(ULetheAbilitySystemComponent* OwnerASC, const FCardViewInfo* InCardInfo)
{
	if (UCardWidget* CreatedCard = CreateWidget<UCardWidget>(this, CardWidgetClass))
	{
		// 만들어진 Card를 OwnerASC와 매핑된 Deck 배열에 추가합니다.
		FDeck& CardWidgets = AbilitySystemComponentToCards.FindOrAdd(OwnerASC);
		CardWidgets.DeckSlots.Reserve(10);

		// Card의 View를 Update한 후 화면에 표시합니다.
		CreatedCard->UpdateCardView(InCardInfo);
		CreatedCard->SetWidgetController(WidgetController);
		CreatedCard->SetOwnerASC(OwnerASC);
		CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardWidgets.DeckSlots.Add(CardSlot);
			CardSlot->SetSize(CARD_SIZE);
		}
	}
}

void UCardPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!AbilitySystemReferences)
	{
		return;
	}

	UpdateDeckPosition(InDeltaTime);
	UpdateHandPosition(InDeltaTime);
}

void UCardPanelWidget::UpdateDeckPosition(const float InDeltaTime)
{
	// ASC를 순서대로 순회합니다.
	for (int32 DeckIndex = 0; DeckIndex < AbilitySystemReferences->Num(); ++DeckIndex)
	{
		// ASC와 매핑된 Deck을 가져옵니다.
		const FAbilitySystemReference& ASCReference = (*AbilitySystemReferences)[DeckIndex];
		FDeck* Deck = AbilitySystemComponentToCards.Find(ASCReference.AbilitySystemComponent);

		// Deck에 카드가 없는 경우 return합니다.
		if (!Deck || Deck->DeckSlots.Num() == 0)
		{
			return;
		}

		// ASC에 해당하는 DeckPosition을 가져옵니다.
		FVector2D TargetPosition = DeckPositions[DeckIndex];

		// 가장 아랫장부터 윗장까지 순회합니다.
		const int32 LastCardIndex = Deck->DeckSlots.Num() - 1;
		for (int32 CardIndex = 0; CardIndex <= LastCardIndex; ++CardIndex)
		{
			UCanvasPanelSlot* CardSlot = Deck->DeckSlots[CardIndex];
			if (!CardSlot)
			{
				return;
			}

			// 목적지를 결정합니다.
			if (CardIndex == LastCardIndex && Deck->bIsHovered)
			{
				// Deck 위에 마우스가 올라가있고 가장 윗장의 위치를 조정할 때 들어오는 분기입니다.
				TargetPosition.Y += DeckYPosGap * 10.f;
			}

			// 카드의 이번 프레임 위치를 결정합니다.
			const FVector2D CurrentPosition = CardSlot->GetPosition();
			if (!CurrentPosition.Equals(TargetPosition, 0.1f))
			{
				// 카드가 아직 목표 지점에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
				const FVector2D FinalCardPosition = FMath::Vector2DInterpTo(CurrentPosition, TargetPosition, InDeltaTime, CardMoveSpeed);
				CardSlot->SetPosition(FinalCardPosition);
			}

			// 층층이 쌓인 모습을 표현할 수 있도록 Y값을 조정합니다.
			TargetPosition.Y -= DeckYPosGap;
		}
	}
}

void UCardPanelWidget::UpdateHandPosition(const float InDeltaTime)
{
	for (UCanvasPanelSlot* CardSlot : Hands)
	{
		// 임시 로직입니다.
		const FVector2D CurrentPosition = CardSlot->GetPosition();
		const FVector2D TargetPosition = FVector2D(0.f, 0.f);
		const FVector2D FinalPosition = FMath::Vector2DInterpTo(CurrentPosition, TargetPosition, InDeltaTime, CardMoveSpeed);
		CardSlot->SetPosition(FinalPosition);
	}
}

void UCardPanelWidget::OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction)
{
	switch (InCardAction)
	{
	case ECardAction::DeckHovered:
		DeckHovered(InCardWidget, true);
		break;
	case ECardAction::DeckUnhovered:
		DeckHovered(InCardWidget, false);
		break;
	case ECardAction::Draw:
		Draw(InCardWidget);
		break;
	case ECardAction::HandHovered:
		break;
	case ECardAction::HandUnhovered:
		break;
	case ECardAction::Use:
		break;
	default:
		break;
	}
}

void UCardPanelWidget::DeckHovered(const UCardWidget* InCardWidget, const bool bInHovered)
{
	if (ULetheAbilitySystemComponent* OwnerASC = Cast<ULetheAbilitySystemComponent>(InCardWidget->GetOwnerASC()))
	{
		if (FDeck* Deck = AbilitySystemComponentToCards.Find(OwnerASC))
		{
			Deck->bIsHovered = bInHovered;
		}
	}
}

void UCardPanelWidget::Draw(UCardWidget* InCardWidget)
{
	if (ULetheAbilitySystemComponent* OwnerASC = Cast<ULetheAbilitySystemComponent>(InCardWidget->GetOwnerASC()))
	{
		if (FDeck* Deck = AbilitySystemComponentToCards.Find(OwnerASC))
		{
			Deck->bIsHovered = false;
			if (UCanvasPanelSlot* DrawnCardSlot = Deck->DeckSlots.Pop(EAllowShrinking::No))
			{
				// 카드를 핸드에 추가하고 그에 맞게 정렬될 수 있도록 합니다.
				const FVector2D CurrentPosition = DrawnCardSlot->GetPosition();
				DrawnCardSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));

				// Anchors가 변경되면 Position의 기준점이 달라져 카드가 순간이동하기 때문에, 이를 보정해 다시 Position을 할당합니다.
				const FVector2D CanvasSize = RootCanvasPanel->GetTickSpaceGeometry().GetLocalSize();
				const FVector2D AnchorShift = FVector2D(CanvasSize.X * 0.5f, CanvasSize.Y * 1.f);
				DrawnCardSlot->SetPosition(CurrentPosition - AnchorShift);
				Hands.Emplace(DrawnCardSlot);

				// CardWidget이 스스로의 상태를 알 수 있도록 알려줍니다.
				InCardWidget->SetCardContainer(ECardContainer::Hand);
			}
		}
	}
}
