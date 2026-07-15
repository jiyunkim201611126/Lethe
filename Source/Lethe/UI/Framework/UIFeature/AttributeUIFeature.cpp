// Copyright JETBLU, Inc. All Rights Reserved.

#include "AttributeUIFeature.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/UI/Battle/Attribute/AttributeWidgetController.h"

ULetheWidgetController* UAttributeUIFeature::CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
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

ULetheWidgetController* UAttributeUIFeature::CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS)
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
