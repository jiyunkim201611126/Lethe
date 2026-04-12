// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheHUD.h"

#include "Blueprint/UserWidget.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/UI/Battle/Attribute/AttributeWidgetController.h"
#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidget.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidgetController.h"

void ULetheHUD::InitPlayerBattleUI(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);

	ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);

	// WidgetController 객체를 생성합니다.
	GetOrCreateOverlayWidgetController();
	// 총 4쌍의 ASC, AS를 WidgetController에게 넘겨줘야 하기 때문에 생성 시점이 아닌 여기서 호출합니다.
	OverlayWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	OverlayWidgetController->BindCallbacks(LetheASC, LetheAS);
	
	GetOrCreateCardWidgetController();
	CardPanelWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	CardPanelWidgetController->BindCallbacks(LetheASC, LetheAS);

	if (!OverlayWidget)
	{
		OverlayWidget = CreateWidget<UOverlayWidget>(GetWorld(), OverlayWidgetClass);
		OverlayWidget->SetWidgetController(OverlayWidgetController);
		OverlayWidget->AddToViewport();
	}
}

ULetheWidgetController* ULetheHUD::CreatePlayerAttributeWidgetController(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	// 각 캐릭터의 AttributeWidget에 Controller를 하나씩 만들어 할당합니다.
	UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, PlayerAttributeWidgetControllerClass);
	if (AttributeWidgetController)
	{
		const FWidgetControllerParams WidgetControllerParams(PC, PS, ASC, AS);
		ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
		ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);
		AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		AttributeWidgetController->BindCallbacks(LetheASC, LetheAS);
		return AttributeWidgetController;
	}
	return nullptr;
}

ULetheWidgetController* ULetheHUD::CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);
	UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, AttributeWidgetControllerClass);
	if (AttributeWidgetController)
	{
		const FWidgetControllerParams WidgetControllerParams(PC, nullptr, ASC, AS);
		AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		AttributeWidgetController->BindCallbacks(LetheASC, LetheAS);
		return AttributeWidgetController;
	}
	return nullptr;
}

UOverlayWidgetController* ULetheHUD::GetOrCreateOverlayWidgetController()
{
	if (!OverlayWidgetController)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
	}
	return OverlayWidgetController;
}

UCardPanelWidgetController* ULetheHUD::GetOrCreateCardWidgetController()
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
