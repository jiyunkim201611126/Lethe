// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheGameUIFeature.h"
#include "CardPanelUIFeature.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UAttributeWidgetController;
class UCardPanelWidget;
class UCardPanelWidgetController;
class UCardViewData;
class UOverlayWidget;
class UOverlayWidgetController;
class UViewCardDetailWidgetController;

UCLASS(Abstract, Blueprintable)
class LETHE_API UCardPanelUIFeature : public ULetheGameUIFeature
{
	GENERATED_BODY()

public:
	virtual void DeinitializeFeature() override;
	
	/** OverlayWidget, CardPanelWidget, CardActor 등의 초기화를 담당하는 함수로, PlayerCharacter BeginPlay 타이밍에 호출(최대 총 4번)합니다. */
	void InitializePlayerCardUI(APlayerController* PC, UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS);

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
