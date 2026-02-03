// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LetheHUD.generated.h"

class UAttributeWidgetController;
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
	void InitPlayerUI(APlayerController* PC, APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS, UUserWidget* InAttributeWidget);
	void InitEnemyUI(UAbilitySystemComponent* ASC, UAttributeSet* AS, UUserWidget* InAttributeWidget);
	
	UOverlayWidgetController* CreateOverlayWidgetController();
	UCardPanelWidgetController* CreateCardWidgetController();
	
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
	
private:
	UPROPERTY()
	TObjectPtr<UOverlayWidget> OverlayWidget;

	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;

	UPROPERTY()
	TObjectPtr<UCardPanelWidgetController> CardPanelWidgetController;
};
