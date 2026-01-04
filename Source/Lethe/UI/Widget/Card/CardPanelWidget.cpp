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
}

void UCardPanelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!AbilitySystemReferences)
	{
		return;
	}

	// TMap 반복문은 순서를 보장하지 않기 때문에 Index 기반 for문을 사용합니다.
	// 총 40번의 순회를 거칩니다.
	for (int32 Index = 0; Index < PLAYABLE_CHARACTER_NUMBER; ++Index)
	{
		if (AbilitySystemReferences->IsValidIndex(Index))
		{
			FVector2D CurrentDeckPosition = DeckPositions[Index];
			const FCardWidgets& CardWidgets = AbilitySystemComponentToCards.FindRef((*AbilitySystemReferences)[Index].AbilitySystemComponent);
			for (UCardWidget* CardWidget : CardWidgets.CardWidgets)
			{
				CurrentDeckPosition.Y += DeckYPosGap;
				CardWidget->SetRenderTranslation(CurrentDeckPosition);
			}
		}
	}
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

void UCardPanelWidget::CreateCard(UAbilitySystemComponent* OwnerASC, const FCardViewInfo* InCardInfo)
{
	if (UCardWidget* CreatedCard = CreateWidget<UCardWidget>(this, CardWidgetClass))
	{
		// 만들어진 Card를 OwnerASC와 매핑합니다.
		FCardWidgets& CardWidgets = AbilitySystemComponentToCards.FindOrAdd(OwnerASC);
		CardWidgets.CardWidgets.Add(CreatedCard);

		// Card의 View를 Update한 후 화면에 표시합니다.
		CreatedCard->UpdateCardView(InCardInfo);
		CreatedCard->SetWidgetController(WidgetController);
		if (UCanvasPanelSlot* CardSlot = RootCanvasPanel->AddChildToCanvas(CreatedCard))
		{
			CardSlot->SetSize(CARD_SIZE);
		}
	}
}
