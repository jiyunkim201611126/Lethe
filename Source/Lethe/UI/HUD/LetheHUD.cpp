// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheHUD.h"

#include "Blueprint/UserWidget.h"
#include "Lethe/UI/Widget/OverlayWidget.h"
#include "Lethe/UI/WidgetController/CardPanelWidgetController.h"
#include "Lethe/UI/WidgetController/OverlayWidgetController.h"

void ALetheHUD::InitHUD(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	check(OverlayWidgetClass);
	check(OverlayWidgetControllerClass);
	check(CardPanelWidgetControllerClass);

	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);

	// WidgetController 객체를 생성합니다.
	CreateOverlayWidgetController(WidgetControllerParams);
	// 총 4쌍의 ASC, AS를 WidgetController에게 넘겨줘야 하기 때문에 생성 시점이 아닌 여기서 호출합니다.
	OverlayWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	OverlayWidgetController->BindCallbacksToDependencies();
	
	CreateCardWidgetController(WidgetControllerParams);
	CardPanelWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	CardPanelWidgetController->BindCallbacksToDependencies();

	if (!OverlayWidget)
	{
		OverlayWidget = CreateWidget<UOverlayWidget>(GetWorld(), OverlayWidgetClass);
	}

	OverlayWidget->SetWidgetController(OverlayWidgetController);
	OverlayWidget->AddToViewport();
}

UOverlayWidgetController* ALetheHUD::CreateOverlayWidgetController(const FWidgetControllerParams& WidgetControllerParams)
{
	if (!OverlayWidgetController)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
	}
	return OverlayWidgetController;
}

UCardPanelWidgetController* ALetheHUD::CreateCardWidgetController(const FWidgetControllerParams& WidgetControllerParams)
{
	if (!CardPanelWidgetController)
	{
		CardPanelWidgetController = NewObject<UCardPanelWidgetController>(this, CardPanelWidgetControllerClass);
	}
	return CardPanelWidgetController;
}

UOverlayWidgetController* ALetheHUD::GetOverlayWidgetController() const
{
	return OverlayWidgetController;
}

UCardPanelWidgetController* ALetheHUD::GetCardPanelWidgetController() const
{
	return CardPanelWidgetController;
}
