// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameUIFeature.h"
#include "AttributeUIFeature.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UAttributeWidgetController;
class ULetheWidgetController;

UCLASS(Abstract, Blueprintable)
class LETHE_API UAttributeUIFeature : public ULetheGameUIFeature
{
	GENERATED_BODY()

public:
	/** AttributeWidget을 가진 캐릭터가 BeginPlay 타이밍에 호출합니다. */
	ULetheWidgetController* CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS);

protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UAttributeWidgetController> EnemyAttributeWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UAttributeWidgetController> PlayerAttributeWidgetControllerClass;
};
