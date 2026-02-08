// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheHUD.h"

#include "Blueprint/UserWidget.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/UI/Widget/Attribute/AttributeWidget.h"
#include "Lethe/UI/Widget/Attribute/AttributeWidgetController.h"
#include "Lethe/UI/Widget/BattleWidget/CardPanelWidgetController.h"
#include "Lethe/UI/Widget/Overlay/OverlayWidget.h"
#include "Lethe/UI/Widget/Overlay/OverlayWidgetController.h"

void ULetheHUD::InitPlayerUI(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS, UUserWidget* InAttributeWidget)
{
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);

	ULetheAbilitySystemComponent* LetheASC = Cast<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = Cast<ULetheAttributeSet>(AS);

	// WidgetController 객체를 생성합니다.
	CreateOverlayWidgetController();
	// 총 4쌍의 ASC, AS를 WidgetController에게 넘겨줘야 하기 때문에 생성 시점이 아닌 여기서 호출합니다.
	OverlayWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	OverlayWidgetController->BindCallbacks(LetheASC, LetheAS);
	
	CreateCardWidgetController();
	CardPanelWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	CardPanelWidgetController->BindCallbacks(LetheASC, LetheAS);

	if (!OverlayWidget)
	{
		OverlayWidget = CreateWidget<UOverlayWidget>(GetWorld(), OverlayWidgetClass);
		OverlayWidget->SetWidgetController(OverlayWidgetController);
		OverlayWidget->AddToViewport();
	}

	// 각 캐릭터의 AttributeWidget에 Controller를 하나씩 만들어 할당합니다.
	UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, PlayerAttributeWidgetControllerClass);
	UAttributeWidget* AttributeWidget = Cast<UAttributeWidget>(InAttributeWidget);
	if (AttributeWidgetController && AttributeWidget)
	{
		AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		AttributeWidgetController->BindCallbacks(LetheASC, LetheAS);
		AttributeWidget->SetWidgetController(AttributeWidgetController);
	}
}

void ULetheHUD::InitEnemyUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UUserWidget* InAttributeWidget)
{
	ULetheAbilitySystemComponent* LetheASC = Cast<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = Cast<ULetheAttributeSet>(AS);
	UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, AttributeWidgetControllerClass);
	UAttributeWidget* AttributeWidget = Cast<UAttributeWidget>(InAttributeWidget);
	if (AttributeWidgetController && AttributeWidget)
	{
		const FWidgetControllerParams WidgetControllerParams(PC, nullptr, ASC, AS);
		AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		AttributeWidgetController->BindCallbacks(LetheASC, LetheAS);
		AttributeWidget->SetWidgetController(AttributeWidgetController);
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
