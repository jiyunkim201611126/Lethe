// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheHUD.generated.h"

struct FWidgetControllerParams;
class UCardViewData;
class UAttributeSet;
class UAbilitySystemComponent;
class UOverlayWidget;
class UCardPanelWidget;
class UOverlayWidgetController;
class UCardPanelWidgetController;

UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class LETHE_API ULetheHUD : public UObject
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
