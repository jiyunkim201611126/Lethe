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

	check(DeckTranslations.Num() == PLAYABLE_CHARACTER_NUMBER);
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
		CardSize = CardPanelWidgetController->GetCardSize();
		CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
	}
}

void UCardPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!AbilitySystemReferences)
	{
		return;
	}

	UpdateDeckTranslation(InDeltaTime);
	UpdateHandTransform(InDeltaTime);
}

void UCardPanelWidget::UpdateDeckTranslation(const float InDeltaTime)
{
	// ASC를 순서대로 순회합니다.
	for (int32 DeckIndex = 0; DeckIndex < AbilitySystemReferences->Num(); ++DeckIndex)
	{
		// ASC와 매핑된 Deck을 가져옵니다.
		const FAbilitySystemReference& ASCReference = (*AbilitySystemReferences)[DeckIndex];
		FDeck* Deck = AbilitySystemComponentToCards.Find(ASCReference.AbilitySystemComponent);

		// Deck에 카드가 없는 경우 return합니다.
		if (!Deck || Deck->Deck.Num() == 0)
		{
			return;
		}

		// ASC에 해당하는 DeckTranslation을 가져옵니다.
		FVector2D TargetCardTranslation = DeckTranslations[DeckIndex];

		// 가장 아랫장부터 윗장까지 순회합니다.
		const int32 LastCardIndex = Deck->Deck.Num() - 1;
		for (int32 CardIndex = 0; CardIndex <= LastCardIndex; ++CardIndex)
		{
			UCardWidget* CardWidget = Deck->Deck[CardIndex];
			if (!CardWidget)
			{
				continue;
			}

			// 목적지를 결정합니다.
			if (CardIndex == LastCardIndex && Deck->bIsHovered)
			{
				// Deck 위에 마우스가 올라가있고 가장 윗장의 위치를 조정할 때 들어오는 분기입니다.
				TargetCardTranslation.Y += DeckYTranslationGap * 10.f;
			}

			// 카드의 이번 프레임 위치를 결정합니다.
			const FVector2D CurrentCardTranslation = CardWidget->GetRenderTransform().Translation;
			if (!CurrentCardTranslation.Equals(TargetCardTranslation, 0.1f))
			{
				// 카드가 아직 목표 지점에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
				const FVector2D FinalCardTranslation = FMath::Vector2DInterpTo(CurrentCardTranslation, TargetCardTranslation, InDeltaTime, CardMoveSpeed);
				CardWidget->SetRenderTranslation(FinalCardTranslation);
			}

			// 층층이 쌓인 모습을 표현할 수 있도록 Y값을 조정합니다.
			TargetCardTranslation.Y -= DeckYTranslationGap;
		}
	}
}

void UCardPanelWidget::UpdateHandTransform(const float InDeltaTime)
{
	const int32 CurrentHandsCount = Hands.Num();
	const float CenterIndex = (static_cast<float>(CurrentHandsCount) - 1.f) / 2.f;

	// 현재 마우스가 올라간 카드의 Index를 구합니다.
	int32 HoveredIndex = INDEX_NONE;
	for (int32 Index = 0; Index < CurrentHandsCount; ++Index)
	{
		if (Hands[Index]->ShouldHandHighlight())
		{
			HoveredIndex = Index;
			break;
		}
	}
	
	for (int32 Index = 0; Index < CurrentHandsCount; ++Index)
	{
		UCardWidget* CardWidget = Hands[Index];
		if (!CardWidget)
		{
			continue;
		}
		
		// 1.f씩 차이나는 값을 구한 뒤 유의미한 Translation을 계산합니다.
		const float PivotOffset = static_cast<float>(Index) - CenterIndex;
		FVector2D TargetCardTranslation = PivotOffset * HandTranslationGap;

		// 현재 Hovered된 카드가 있다면 나머지 카드를 밀어냅니다.
		if (HoveredIndex != INDEX_NONE && Index != HoveredIndex)
		{
			// 이 카드가 Hovered된 카드로부터 얼마나 떨어져있는지 구합니다.
			const float IndexGap = Index - HoveredIndex;
			// 밀어낼 방향을 결정합니다.
			const float PushDirection = IndexGap > 0 ? 1.f : -1.f;
			// 밀어낼 수치를 결정하고, 그만큼 밀어냅니다.
			const float PushAmount = HandPushAmount * PushDirection / FMath::Max(1.f, FMath::Abs(IndexGap));
			TargetCardTranslation.X += PushAmount;
		}
		
		const float PivotOffsetAbs = FMath::Abs(PivotOffset);
		// 카드 장수가 짝수일 땐 가운데 2장을 살짝 내리고, 홀수일 땐 가운데 1장을 그보다 좀 더 내립니다. 이렇게 해야 이쁘게 정렬됩니다.
		TargetCardTranslation.Y = FMath::Abs(TargetCardTranslation.Y) + (PivotOffsetAbs <= 1.f ? HandTranslationGap.Y / 2.f : 0.f) + (PivotOffset == 0.f ? HandTranslationGap.Y / 1.5f : 0.f);
		TargetCardTranslation.Y += HandYTranslation;
		const float TargetCardRotation = PivotOffset * HandRotationStepAmount;

		// 카드의 이번 프레임 회전 수치를 결정합니다.
		const FVector2D CurrentCardTranslation = CardWidget->GetRenderTransform().Translation;
		const float CurrentCardRotation = CardWidget->GetRenderTransformAngle();
		if (!FMath::IsNearlyEqual(CurrentCardRotation, TargetCardRotation, 0.05f))
		{
			// 카드가 아직 목표 회전 수치에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
			const float FinalCardRotation = FMath::FInterpTo(CurrentCardRotation, TargetCardRotation, InDeltaTime, CardMoveSpeed);
			CardWidget->SetRenderTransformAngle(FinalCardRotation);
		}

		// 카드 위에 마우스가 있는 경우 크기를 키워서 보여줍니다.
		UpdateHandScale(CardWidget, InDeltaTime);
		
		// 카드의 이번 프레임 위치를 결정합니다
		if (!CurrentCardTranslation.Equals(TargetCardTranslation, 0.05f))
		{
			// 카드가 아직 목표 지점에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
			const FVector2D FinalCardTranslation = FMath::Vector2DInterpTo(CurrentCardTranslation, TargetCardTranslation, InDeltaTime, CardMoveSpeed);
			CardWidget->SetRenderTranslation(FinalCardTranslation);
		}
	}
}

void UCardPanelWidget::UpdateHandScale(UCardWidget* InCardWidget, const float InDeltaTime) const
{
	const FVector2D CurrentCardScale = InCardWidget->GetRenderTransform().Scale;
	const FVector2D TargetCardScale = InCardWidget->ShouldHandHighlight() ? FVector2D(1.f) : HandUnhighlightScale;
	
	// 카드의 이번 크기를 결정합니다
	if (!CurrentCardScale.Equals(TargetCardScale, 0.01f))
	{
		// 카드가 아직 목표 크기에 충분히 가까워지지 않은 경우 들어오는 분기입니다.
		const FVector2D FinalCardScale = FMath::Vector2DInterpTo(CurrentCardScale, TargetCardScale, InDeltaTime, CardMoveSpeed);
		InCardWidget->SetRenderScale(FinalCardScale);
	}
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

void UCardPanelWidget::CreateCard(ULetheAbilitySystemComponent* OwnerASC, const FCardViewInfo* InCardInfo)
{
	if (UCardWidget* CreatedCard = CreateWidget<UCardWidget>(this, CardWidgetClass))
	{
		// 만들어진 Card를 OwnerASC와 매핑된 Deck 배열에 추가합니다.
		FDeck& CardWidgets = AbilitySystemComponentToCards.FindOrAdd(OwnerASC);
		CardWidgets.Deck.Reserve(10);
		CardWidgets.Deck.Add(CreatedCard);

		// Card의 View를 Update한 후 화면에 표시합니다.
		CreatedCard->UpdateCardView(InCardInfo);
		CreatedCard->SetWidgetController(WidgetController);
		CreatedCard->SetOwnerASC(OwnerASC);
		CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);
		
		// Card의 위치, 회전, 크기를 다루기 위한 값들을 설정합니다.
		CreatedCard->SetRenderTransformPivot(FVector2D(0.5f, 0.f));
		CreatedCard->SetRenderScale(HandUnhighlightScale);
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			CardSlot->SetAnchors(FAnchors());
			CardSlot->SetAlignment(FVector2D(0.f, 0.f));
			CardSlot->SetSize(CardSize);
			CardSlot->SetZOrder(DeckZOrder++);
		}
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
			if (UCardWidget* DrawnCardWidget = Deck->Deck.Pop(EAllowShrinking::No))
			{
				// 카드를 핸드에 추가하고 그에 맞게 정렬될 수 있도록 합니다.
				DrawnCardWidget->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
				UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(DrawnCardWidget->Slot);
				CardSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
				CardSlot->SetAlignment(FVector2D(0.5f, 1.f));
				CardSlot->SetZOrder(HandZOrder++);

				// Anchors와 Alignment가 변경되면 Translation의 기준점이 달라져 카드가 순간이동하기 때문에, 이를 보정해 다시 Translation을 할당합니다.
				const FVector2D CanvasSize = RootCanvasPanel->GetTickSpaceGeometry().GetLocalSize();
				const FVector2D AnchorShift = FVector2D(CanvasSize.X * 0.5f, CanvasSize.Y * 1.f);
				const FVector2D CurrentCardTranslation = DrawnCardWidget->GetRenderTransform().Translation;
				const FVector2D OffsetTranslationByCardSize = FVector2D(CardSize.X * 0.5f, CardSize.Y * 1.f);
				DrawnCardWidget->SetRenderTranslation(CurrentCardTranslation - AnchorShift+ OffsetTranslationByCardSize);
				Hands.Emplace(DrawnCardWidget);

				// CardWidget이 스스로의 상태를 알 수 있도록 알려줍니다.
				InCardWidget->SetCardContainer(ECardContainer::Hand);
			}
		}
	}
}
