// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheHUD.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UCardPanelWidget;
class UCardPanelWidgetController;
class UCardViewData;
class UOverlayWidget;
class UOverlayWidgetController;
struct FWidgetControllerParams;

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
