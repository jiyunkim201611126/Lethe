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
	CachedDeckTranslations.Reserve(PLAYABLE_CHARACTER_NUMBER);
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
		
		if (!bCardSizeInitialized)
		{
			for (FVector2D& DeckTranslation : DeckTranslations)
			{
				// 기존 Deck과 Hand에서 다른 Alignment를 사용했었는데, 해상도 변경 시나 보정에 있어 예상치 못 한 결과를 얻을 수 있으므로 이제 Alignment는 고정합니다.
				// 다만 기획자의 편의성을 위해 DeckTranslation을 Alignment에 맞춰 조정해 Deck일 때만 Alignment가 (0.f, 0.f)인 것처럼 동작시킵니다.
				DeckTranslation.X += CardSize.X * 0.5f * UnhighlightScale.X;
				DeckTranslation.Y += CardSize.Y * UnhighlightScale.Y;
			}
			bCardSizeInitialized = true;
		}
		else
		{
			// 사용 편의성 및 성능을 위해 ASC와 DeckTranslation을 매핑합니다.
			CachedDeckTranslations.Reset();
			int32 DeckTranslationIndex = 0;
			for (const auto& AbilitySystemReference : *AbilitySystemReferences)
			{
				CachedDeckTranslations.Emplace(AbilitySystemReference.AbilitySystemComponent, DeckTranslations[DeckTranslationIndex]);
				++DeckTranslationIndex;
			}
		}
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
		UpdateDeckTranslation(InCardWidget->GetOwnerASC());
		UpdateHandsTransform();
		break;
	case ECardAction::HandHovered:
		UpdateHandsTransform();
		break;
	case ECardAction::HandUnhovered:
		UpdateHandsTransform();
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
		CreatedCard->SetRenderTransformPivot(FVector2D(0.5f, 1.f));
		CreatedCard->SetRenderScale(UnhighlightScale);
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			CardSlot->SetAnchors(FAnchors());
			CardSlot->SetAlignment(FVector2D(0.5f, 1.f));
			CardSlot->SetSize(CardSize);
			CardSlot->SetZOrder(DeckZOrder++);
		}

		// 일단 1장 만들어질 때마다 호출하지만,
		// TODO: 추후 덱의 크기가 40장으로 고정되면 40장이 만들어졌을 때 한 번만 호출하도록 변경합니다.
		UpdateAllDeckTranslation();
	}
}

void UCardPanelWidget::UpdateAllDeckTranslation()
{
	// ASC를 순서대로 순회합니다.
	for (const FAbilitySystemReference& AbilitySystemReference : *AbilitySystemReferences)
	{
		// Deck의 위치를 업데이트합니다.
		UpdateDeckTranslation(AbilitySystemReference.AbilitySystemComponent);
	}
}

void UCardPanelWidget::UpdateDeckTranslation(const ULetheAbilitySystemComponent* OwnerASC)
{
	FDeck* Deck = AbilitySystemComponentToCards.Find(OwnerASC);
	// Deck에 카드가 없는 경우 return합니다.
	if (!Deck || Deck->Deck.Num() == 0)
	{
		return;
	}
	
	// ASC에 해당하는 DeckTranslation을 가져옵니다.
	FVector2D TargetCardTranslation = CachedDeckTranslations.FindRef(OwnerASC);
	
	// 가장 윗장부터 아랫장까지 순회합니다.
	for (int32 CardIndex = Deck->Deck.Num() - 1; CardIndex >= 0; --CardIndex)
	{
		UCardWidget* CardWidget = Deck->Deck[CardIndex];
		if (!CardWidget)
		{
			continue;
		}

		// 목적지를 결정합니다.
		FWidgetTransform WidgetTransform = CardWidget->GetRenderTransform();
		WidgetTransform.Translation = TargetCardTranslation;
		WidgetTransform.Scale = UnhighlightScale;
		CardWidget->SetTargetTransform(WidgetTransform);

		// 층층이 쌓인 모습을 표현할 수 있도록 Y값을 조정합니다.
		TargetCardTranslation.Y += DeckYTranslationGap;
	}
}

void UCardPanelWidget::OnDeckHovered(const UCardWidget* InCardWidget, const bool bInHovered)
{
	if (ULetheAbilitySystemComponent* OwnerASC = InCardWidget->GetOwnerASC())
	{
		if (FDeck* Deck = AbilitySystemComponentToCards.Find(OwnerASC))
		{
			// 덱 가장 윗장을 가져옵니다.
			if (UCardWidget* DeckOnTopCard = Deck->Deck.Last())
			{
				// 마우스 Hovered 여부에 따라 CardWidget의 위치를 결정합니다.
				const FVector2D& DeckTranslation = CachedDeckTranslations.FindRef(OwnerASC);
				FWidgetTransform WidgetTransform;
				WidgetTransform.Translation = bInHovered ? DeckTranslation + FVector2D(0.f, DeckYTranslationGap) * 10.f : DeckTranslation;
				WidgetTransform.Scale = UnhighlightScale;
				DeckOnTopCard->SetTargetTransform(WidgetTransform);
			}
		}
	}
}

void UCardPanelWidget::Draw(UCardWidget* InCardWidget)
{
	if (ULetheAbilitySystemComponent* OwnerASC = InCardWidget->GetOwnerASC())
	{
		if (FDeck* Deck = AbilitySystemComponentToCards.Find(OwnerASC))
		{
			if (UCardWidget* DrawnCardWidget = Deck->Deck.Pop(EAllowShrinking::No))
			{
				// 카드를 핸드에 추가하고 그에 맞게 정렬될 수 있도록 합니다.
				UCanvasPanelSlot* CardSlot = Cast<UCanvasPanelSlot>(DrawnCardWidget->Slot);
				CardSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
				CardSlot->SetZOrder(HandZOrder++);

				// Anchors가 변경되면 Translation의 기준점이 달라져 카드가 순간이동하기 때문에, 이를 보정해 다시 Translation을 할당합니다.
				const FVector2D CanvasSize = RootCanvasPanel->GetTickSpaceGeometry().GetLocalSize();
				const FVector2D AnchorShift = FVector2D(CanvasSize.X * 0.5f, CanvasSize.Y * 1.f);
				const FVector2D CurrentCardTranslation = DrawnCardWidget->GetRenderTransform().Translation;
				DrawnCardWidget->SetRenderTranslation(CurrentCardTranslation - AnchorShift);
				Hands.Emplace(DrawnCardWidget);

				InCardWidget->SetCardContainer(ECardContainer::Hand);
			}
		}
	}
}

void UCardPanelWidget::UpdateHandsTransform()
{
	const int32 CurrentHandsCount = Hands.Num();
	const float CenterIndex = (static_cast<float>(CurrentHandsCount) - 1.f) / 2.f;

	// 현재 마우스가 올라간 카드의 Index를 구합니다.
	int32 HoveredIndex = INDEX_NONE;
	for (int32 Index = 0; Index < CurrentHandsCount; ++Index)
	{
		if (Hands[Index]->GetHandHighlightState())
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

		// 현재 Hovered된 카드가 있다면 나머지 카드를 좌우로 밀어냅니다.
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
		
		// 카드의 회전 수치를 결정합니다.
		const float TargetCardAngle = PivotOffset * HandRotationStepAmount;

		// 마우스 Hovered 상태에 따른 카드의 크기를 결정합니다.
		const FVector2D TargetCardScale = Index == HoveredIndex ? FVector2D(1.f) : UnhighlightScale;
		
		FWidgetTransform WidgetTransform;
		WidgetTransform.Translation = TargetCardTranslation;
		WidgetTransform.Angle = TargetCardAngle;
		WidgetTransform.Scale = TargetCardScale;
		CardWidget->SetTargetTransform(WidgetTransform);
	}
}
