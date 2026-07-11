// Copyright JETBLU, Inc. All Rights Reserved.

#include "BattleUIFeature.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/UI/Battle/Attribute/AttributeWidgetController.h"
#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Lethe/UI/Battle/Card/ViewCardDetailWidgetController.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidget.h"
#include "Lethe/UI/Battle/Overlay/OverlayWidgetController.h"
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UBattleUIFeature::DeinitializeFeature()
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

void UBattleUIFeature::InitPlayerBattleUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
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

	if (!OverlayWidget)
	{
		if (LayoutWidget.IsValid())
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
}

ULetheWidgetController* UBattleUIFeature::CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
{
	// 각 캐릭터의 AttributeWidget에 Controller를 하나씩 만들어 할당합니다.
	const TSubclassOf<UAttributeWidgetController> LoadedWidgetClass = PlayerAttributeWidgetControllerClass.LoadSynchronous();
	if (ensure(LoadedWidgetClass))
	{
		UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, LoadedWidgetClass);
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
	}
	return nullptr;
}

ULetheWidgetController* UBattleUIFeature::CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	const TSubclassOf<UAttributeWidgetController> LoadedWidgetClass = EnemyAttributeWidgetControllerClass.LoadSynchronous();
	if (ensure(LoadedWidgetClass))
	{
		UAttributeWidgetController* AttributeWidgetController = NewObject<UAttributeWidgetController>(this, LoadedWidgetClass);
		if (AttributeWidgetController)
		{
			const FWidgetControllerParams WidgetControllerParams(PC, ASC, AS, nullptr);
			ULetheAbilitySystemComponent* LetheASC = CastChecked<ULetheAbilitySystemComponent>(ASC);
			ULetheAttributeSet* LetheAS = CastChecked<ULetheAttributeSet>(AS);
			AttributeWidgetController->SetWidgetControllerParams(WidgetControllerParams);
			AttributeWidgetController->BindCallbacks(LetheASC, LetheAS, nullptr);
			return AttributeWidgetController;
		}
	}
	return nullptr;
}

UOverlayWidgetController* UBattleUIFeature::GetOrCreateOverlayWidgetController()
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

UCardPanelWidgetController* UBattleUIFeature::GetOrCreateCardPanelWidgetController()
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

UViewCardDetailWidgetController* UBattleUIFeature::GetOrCreateViewCardDetailWidgetController()
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
