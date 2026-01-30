// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheHUD.h"

#include "Blueprint/UserWidget.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/UI/Widget/OverlayWidget.h"
#include "Lethe/UI/Widget/BattleWidget/CardPanelWidget.h"
#include "Lethe/UI/WidgetController/CardPanelWidgetController.h"
#include "Lethe/UI/WidgetController/OverlayWidgetController.h"

void ULetheHUD::InitHUD(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	check(OverlayWidgetClass);
	check(OverlayWidgetControllerClass);
	check(CardPanelWidgetControllerClass);

	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);

	ULetheAbilitySystemComponent* LetheASC = Cast<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = Cast<ULetheAttributeSet>(AS);

	// WidgetController 객체를 생성합니다.
	CreateOverlayWidgetController();
	// 총 4쌍의 ASC, AS를 WidgetController에게 넘겨줘야 하기 때문에 생성 시점이 아닌 여기서 호출합니다.
	OverlayWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	OverlayWidgetController->BindCallbacksToDependencies(LetheASC, LetheAS);
	
	CreateCardWidgetController();
	CardPanelWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	CardPanelWidgetController->BindCallbacksToDependencies(LetheASC, LetheAS);

	if (!OverlayWidget)
	{
		OverlayWidget = CreateWidget<UOverlayWidget>(GetWorld(), OverlayWidgetClass);
		OverlayWidget->SetWidgetController(OverlayWidgetController);
		OverlayWidget->AddToViewport();
	}
}

UOverlayWidgetController* ULetheHUD::CreateOverlayWidgetController()
{
	if (!OverlayWidgetController)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
	}
	return OverlayWidgetController;
}

UCardPanelWidgetController* ULetheHUD::CreateCardWidgetController()
{
	if (!CardPanelWidgetController)
	{
		CardPanelWidgetController = NewObject<UCardPanelWidgetController>(this, CardPanelWidgetControllerClass);
	}
	return CardPanelWidgetController;
}

UOverlayWidgetController* ULetheHUD::GetOverlayWidgetController() const
{
	return OverlayWidgetController;
}

UCardPanelWidgetController* ULetheHUD::GetCardPanelWidgetController() const
{
	return CardPanelWidgetController;
}
