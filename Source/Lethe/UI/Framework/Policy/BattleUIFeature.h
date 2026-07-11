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
	virtual void DeinitializeFeature() override;
	
	/** OverlayWidget, CardPanelWidget, CardActor 등의 초기화를 담당하는 함수로, PlayerCharacter BeginPlay 타이밍에 호출(최대 총 4번)합니다. */
	void InitPlayerBattleUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);

	/** AttributeWidget을 가진 캐릭터가 BeginPlay 타이밍에 호출합니다. */
	ULetheWidgetController* CreatePlayerAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);
	ULetheWidgetController* CreateEnemyAttributeWidgetController(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS);

private:
	UOverlayWidgetController* GetOrCreateOverlayWidgetController();
	UCardPanelWidgetController* GetOrCreateCardPanelWidgetController();
	UViewCardDetailWidgetController* GetOrCreateViewCardDetailWidgetController();

protected:
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UOverlayWidget> OverlayWidgetClass;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UCardPanelWidgetController> CardPanelWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UViewCardDetailWidgetController> ViewCardDetailWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UAttributeWidgetController> EnemyAttributeWidgetControllerClass;

	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UAttributeWidgetController> PlayerAttributeWidgetControllerClass;
	
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
