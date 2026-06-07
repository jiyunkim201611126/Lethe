// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheHUD.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UAttributeWidgetController;
class UCardPanelWidget;
class UCardPanelWidgetController;
class UCardViewData;
class ULetheWidgetController;
class UOverlayWidget;
class UOverlayWidgetController;
class UPlayerAttributeWidgetController;
struct FWidgetControllerParams;

UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class LETHE_API ULetheHUD : public UObject
{
	GENERATED_BODY()

public:
	/** AttributeWidget, CardActor 초기화를 위해 필요한 변수를 PlayerCharacter BeginPlay 타이밍마다 한 번씩 호출(최대 총 4번)합니다. */
	void InitPlayerBattleUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UOverlayWidgetController* GetOrCreateOverlayWidgetController();
	UCardPanelWidgetController* GetOrCreateCardWidgetController();
	
	UOverlayWidgetController* GetOverlayWidgetController() const;
	UCardPanelWidgetController* GetCardPanelWidgetController() const;

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UOverlayWidget> OverlayWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCardPanelWidgetController> CardPanelWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeWidgetController> AttributeWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UAttributeWidgetController> PlayerAttributeWidgetControllerClass;
	
private:
	UPROPERTY()
	TObjectPtr<UOverlayWidget> OverlayWidget;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;
};
