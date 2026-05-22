// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheHUD.h"

#include "Blueprint/UserWidget.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/UI/Battle/Attribute/AttributeWidgetController.h"
#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidget.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidgetController.h"

void ULetheHUD::InitPlayerBattleUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
{
	const FWidgetControllerParams WidgetControllerParams(PC, ASC, AS, PAS);

	ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
	ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);
	UPlayerAttributeSet* PlayerAS = CastChecked<UPlayerAttributeSet>(PAS);

	// WidgetController 객체를 생성합니다.
	GetOrCreateOverlayWidgetController();
	// 총 4쌍의 ASC, AS를 WidgetController에게 넘겨줘야 하기 때문에 생성 시점이 아닌 여기서 호출합니다.
	OverlayWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	OverlayWidgetController->BindCallbacks(LetheASC, LetheAS, PlayerAS);
	
	GetOrCreateCardWidgetController();
	CardPanelWidgetController->SetWidgetControllerParams(WidgetControllerParams);
	CardPanelWidgetController->BindCallbacks(LetheASC, LetheAS, PlayerAS);

	if (!OverlayWidget)
	{
		OverlayWidget = CreateWidget<UOverlayWidget>(GetWorld(), OverlayWidgetClass);
		OverlayWidget->SetWidgetController(OverlayWidgetController);
		OverlayWidget->AddToViewport();
	}
}

ULetheWidgetController* ULetheHUD::CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
{
	// 각 캐릭터의 AttributeWidget에 Controller를 하나씩 만들어 할당합니다.
	UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, PlayerAttributeWidgetControllerClass);
	if (AttributeWidgetController)
	{
		const FWidgetControllerParams WidgetControllerParams(PC, ASC, AS, PAS);
		ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
		ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);
		UPlayerAttributeSet* PlayerAS = Cast<UPlayerAttributeSet>(PAS);
		AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		AttributeWidgetController->BindCallbacks(LetheASC, LetheAS, PlayerAS);
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
		const FWidgetControllerParams WidgetControllerParams(PC, ASC, AS, nullptr);
		AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
		AttributeWidgetController->BindCallbacks(LetheASC, LetheAS, nullptr);
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
