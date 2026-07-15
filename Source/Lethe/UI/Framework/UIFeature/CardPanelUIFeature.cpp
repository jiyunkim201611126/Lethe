// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelUIFeature.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Lethe/UI/Battle/Card/ViewCardDetailWidgetController.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidget.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidgetController.h"
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UCardPanelUIFeature::DeinitializeFeature()
{
	if (OverlayWidget)
	{
		OverlayWidget->DeactivateWidget();
		OverlayWidget->RemoveFromParent();
		OverlayWidget = nullptr;
	}

	OverlayWidgetController = nullptr;
	CardPanelWidgetController = nullptr;
	ViewCardDetailWidgetController = nullptr;
	
	Super::DeinitializeFeature();
}

void UCardPanelUIFeature::InitializePlayerCardUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
{
	const FWidgetControllerParams WidgetControllerParams(PC, ASC, AS, PAS);

	ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);
	UPlayerAttributeSet* PlayerAS = CastChecked<UPlayerAttributeSet>(PAS);

	// WidgetController 객체를 생성합니다.
	GetOrCreateOverlayWidgetController();
	// 총 4쌍의 ASC, AS를 WidgetController에게 넘겨줘야 하기 때문에 생성 시점이 아닌 여기서 호출합니다.
	if (OverlayWidgetController)
	{
		OverlayWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		OverlayWidgetController->BindCallbacks(LetheASC, LetheAS, PlayerAS);
	}
	
	GetOrCreateCardPanelWidgetController();
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		CardPanelWidgetController->BindCallbacks(LetheASC, LetheAS, PlayerAS);
	}

	GetOrCreateViewCardDetailWidgetController();
	if (ViewCardDetailWidgetController)
	{
		ViewCardDetailWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		ViewCardDetailWidgetController->BindCallbacks(LetheASC, LetheAS, PlayerAS);
	}

	if (!OverlayWidgetController || !CardPanelWidgetController || !ViewCardDetailWidgetController)
	{
		return;
	}

	if (!OverlayWidget && LayoutWidget.IsValid())
	{
		const TSubclassOf<UOverlayWidget> LoadedWidgetClass = OverlayWidgetClass.LoadSynchronous();
		if (ensure(LoadedWidgetClass))
		{
			OverlayWidget = LayoutWidget->PushWidgetToLayerStack<UOverlayWidget>(FLetheGameplayTags::Get().UI_Layer_Game, LoadedWidgetClass,
				[this](UOverlayWidget& WidgetToInit)
				{
					WidgetToInit.SetBattleWidgetControllers(CardPanelWidgetController, ViewCardDetailWidgetController);
					WidgetToInit.SetWidgetController(OverlayWidgetController);
				});
		}
	}
}

UOverlayWidgetController* UCardPanelUIFeature::GetOrCreateOverlayWidgetController()
{
	if (!OverlayWidgetController)
	{
		const TSubclassOf<UOverlayWidgetController> LoadedControllerClass = OverlayWidgetControllerClass.LoadSynchronous();
		if (ensure(LoadedControllerClass))
		{
			OverlayWidgetController = NewObject<UOverlayWidgetController>(this, LoadedControllerClass);
		}
	}
	return OverlayWidgetController;
}

UCardPanelWidgetController* UCardPanelUIFeature::GetOrCreateCardPanelWidgetController()
{
	if (!CardPanelWidgetController)
	{
		const TSubclassOf<UCardPanelWidgetController> LoadedControllerClass = CardPanelWidgetControllerClass.LoadSynchronous();
		if (ensure(LoadedControllerClass))
		{
			CardPanelWidgetController = NewObject<UCardPanelWidgetController>(this, LoadedControllerClass);
		}
	}
	return CardPanelWidgetController;
}

UViewCardDetailWidgetController* UCardPanelUIFeature::GetOrCreateViewCardDetailWidgetController()
{
	if (!ViewCardDetailWidgetController)
	{
		const TSubclassOf<UViewCardDetailWidgetController> LoadedControllerClass = ViewCardDetailWidgetControllerClass.LoadSynchronous();
		if (ensure(LoadedControllerClass))
		{
			ViewCardDetailWidgetController = NewObject<UViewCardDetailWidgetController>(this, LoadedControllerClass);
		}
	}
	return ViewCardDetailWidgetController;
}
