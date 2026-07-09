// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameUIFeature.h"
#include "BattleUIFeature.generated.h"

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
class UViewCardDetailWidgetController;
struct FWidgetControllerParams;

/**
 * 배틀 화면에 필요한 WidgetController들을 생성하고 Overlay에 꽂아주는 역할을 수행합니다.
 */
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class LETHE_API UBattleUIFeature : public ULetheGameUIFeature
{
	GENERATED_BODY()

public:
	virtual void InitializeFeature(ULethePrimaryGameLayout* InLayoutWidget) override;
	
	/** AttributeWidget, CardActor 초기화를 위해 필요한 변수를 PlayerCharacter BeginPlay 타이밍마다 한 번씩 호출(최대 총 4번)합니다. */
	void InitPlayerBattleUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS);

private:
	UOverlayWidgetController* GetOrCreateOverlayWidgetController();
	UCardPanelWidgetController* GetOrCreateCardPanelWidgetController();
	UViewCardDetailWidgetController* GetOrCreateViewCardDetailWidgetController();

protected:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UOverlayWidget> OverlayWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UCardPanelWidgetController> CardPanelWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UViewCardDetailWidgetController> ViewCardDetailWidgetControllerClass;

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

	UPROPERTY()
	TObjectPtr<UViewCardDetailWidgetController> ViewCardDetailWidgetController;
};
