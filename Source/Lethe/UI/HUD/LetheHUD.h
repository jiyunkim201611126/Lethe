// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "LetheHUD.generated.h"

struct FWidgetControllerParams;
class UAttributeSet;
class UAbilitySystemComponent;
class UOverlayWidget;
class UOverlayWidgetController;
class UCardPanelWidgetController;

UCLASS()
class LETHE_API ALetheHUD : public AHUD
{
	GENERATED_BODY()

public:
	void InitHUD(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS);
	
	UOverlayWidgetController* CreateOverlayWidgetController(const FWidgetControllerParams& WidgetControllerParams);
	UCardPanelWidgetController* CreateCardWidgetController(const FWidgetControllerParams& WidgetControllerParams);
	
	UOverlayWidgetController* GetOverlayWidgetController() const;
	UCardPanelWidgetController* GetCardPanelWidgetController() const;
	
private:
	UPROPERTY()
	TObjectPtr<UOverlayWidget> OverlayWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidget> OverlayWidgetClass;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass;

	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UCardPanelWidgetController> CardPanelWidgetControllerClass;
};
