// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LetheHUD.generated.h"

class UCardViewData;
struct FWidgetControllerParams;
class UAttributeSet;
class UAbilitySystemComponent;
class UOverlayWidget;
class UCardPanelWidget;
class UOverlayWidgetController;
class UCardPanelWidgetController;

UCLASS()
class LETHE_API ALetheHUD : public AHUD
{
	GENERATED_BODY()

public:
	void InitHUD(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UOverlayWidgetController* CreateOverlayWidgetController();
	UCardPanelWidgetController* CreateCardWidgetController();
	
	UOverlayWidgetController* GetOverlayWidgetController() const;
	UCardPanelWidgetController* GetCardPanelWidgetController() const;

protected:
	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidget> OverlayWidgetClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCardPanelWidgetController> CardPanelWidgetControllerClass;
	
private:
	UPROPERTY()
	TObjectPtr<UOverlayWidget> OverlayWidget;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;
};
