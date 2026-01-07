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
		
		// Slot의 Anchor나 Alignment와는 다른, Rotation을 다루기 위한 Pivot을 설정합니다.
		CreatedCard->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			// Card의 Position을 다루기 위한 값을 설정합니다.
			CardSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CardWidgets.DeckSlots.Add(CardSlot);
			CardSlot->SetSize(CARD_SIZE);
			CardSlot->SetZOrder(DeckZOrder++);
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
		FVector2D TargetCardPosition = DeckPositions[DeckIndex];

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
				TargetCardPosition.Y += DeckYPosGap * 10.f;
			}

			// 카드의 이번 프레임 위치를 결정합니다.
			const FVector2D CurrentCardPosition = CardSlot->GetPosition();
			if (!CurrentCardPosition.Equals(TargetCardPosition, 0.1f))
			{
				// 카드가 아직 목표 지점에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
				const FVector2D FinalCardPosition = FMath::Vector2DInterpTo(CurrentCardPosition, TargetCardPosition, InDeltaTime, CardMoveSpeed);
				CardSlot->SetPosition(FinalCardPosition);
			}

			// 층층이 쌓인 모습을 표현할 수 있도록 Y값을 조정합니다.
			TargetCardPosition.Y -= DeckYPosGap;
		}
	}
}

void UCardPanelWidget::UpdateHandPosition(const float InDeltaTime)
{
	const int32 CurrentHandsCount = Hands.Num();
	const float CenterIndex = (static_cast<float>(CurrentHandsCount) - 1.f) / 2.f;
	
	for (int32 Index = 0; Index < CurrentHandsCount; ++Index)
	{
		UCanvasPanelSlot* CardSlot = Hands[Index];
		if (!CardSlot)
		{
			return;
		}
		
		// 1.f씩 차이나는 값을 구한 뒤 유의미한 Position을 계산합니다.
		const float PivotOffset = static_cast<float>(Index) - CenterIndex;
		FVector2D TargetCardPosition = PivotOffset * HandPosGap;
		const float PivotOffsetAbs = FMath::Abs(PivotOffset);
		// 카드 장수가 짝수일 땐 가운데 2장을 살짝 내리고, 홀수일 땐 가운데 1장을 그보다 좀 더 내립니다. 이렇게 해야 이쁘게 정렬됩니다.
		TargetCardPosition.Y = FMath::Abs(TargetCardPosition.Y) - (PivotOffsetAbs <= 1.f ? 140.f : 150.f) + (PivotOffsetAbs == 0.f ? 13.f : 0.f);
		const float TargetCardRotation = PivotOffset * HandRotationStepAmount;

		// 카드의 이번 프레임 회전 수치를 결정합니다.
		const FVector2D CurrentCardPosition = CardSlot->GetPosition();
		if (UWidget* Widget = CardSlot->GetContent())
		{
			const float CurrentCardRotation = Widget->GetRenderTransformAngle();
			if (!FMath::IsNearlyEqual(CurrentCardRotation, TargetCardRotation, 0.05f))
			{
				// 카드가 아직 목표 회전 수치에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
				const float FinalCardRotation = FMath::FInterpTo(CurrentCardRotation, TargetCardRotation, InDeltaTime, CardMoveSpeed);
				Widget->SetRenderTransformAngle(FinalCardRotation);
			}

			// 카드 위에 마우스가 있는 경우 크기를 키워서 보여줍니다.
			if (UCardWidget* CardWidget = Cast<UCardWidget>(Widget))
			{
				if (CardWidget->ShouldHandHighlight())
				{
					UpdateHandScale(CardWidget, InDeltaTime);
				}
			}
		}
		
		// 카드의 이번 프레임 위치를 결정합니다
		if (!CurrentCardPosition.Equals(TargetCardPosition, 0.05f))
		{
			// 카드가 아직 목표 지점에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
			const FVector2D FinalCardPosition = FMath::Vector2DInterpTo(CurrentCardPosition, TargetCardPosition, InDeltaTime, CardMoveSpeed);
			CardSlot->SetPosition(FinalCardPosition);
		}
	}
}

void UCardPanelWidget::UpdateHandScale(UWidget* InCardWidget, const float InDeltaTime)
{
	FVector2D CurrentCardScale = InCardWidget->GetRenderTransform().Scale;
}

void UCardPanelWidget::OnCardMouseEvent(UCardWidget* InCardWidget, const ECardAction InCardAction)
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

void UCardPanelWidget::OnDeckHovered(const UCardWidget* InCardWidget, const bool bInHovered)
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
				DrawnCardSlot->SetZOrder(HandZOrder++);

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
