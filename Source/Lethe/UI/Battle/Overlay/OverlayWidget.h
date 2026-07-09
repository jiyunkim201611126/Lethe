// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheActivatableWidget.h"
#include "OverlayWidget.generated.h"

class ACardActor;
class UCardPanelWidget;
class UInputAction;
class UViewCardDetailWidget;

UCLASS(Abstract)
class LETHE_API UOverlayWidget : public ULetheActivatableWidget
{
	GENERATED_BODY()

public:
	void SetBattleWidgetControllers(ULetheWidgetController* InCardPanelWidgetController, ULetheWidgetController* InViewCardDetailWidgetController);

protected:
	//~ Begin UCommonActivatableWidget Interface
	virtual void NativeOnActivated() override;
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End of UCommonActivatableWidget Interface

private:
	void OnStartViewCardDetail(const ACardActor* CardActor) const;
	
	void HandleKeyboard1() const;
	void HandleKeyboard2() const;
	void HandleKeyboard3() const;
	void HandleKeyboard4() const;
	void HandleKeyboard5() const;
	void HandleKeyboard6() const;
	void HandleKeyboard7() const;
	void HandleKeyboard8() const;
	void HandleKeyboard9() const;
	void HandleKeyboard0() const;

protected:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCardPanelWidget> CardPanel;

	UPROPERTY(EditDefaultsOnly, Category = "View")
	TSoftClassPtr<UViewCardDetailWidget> ViewCardDetailWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard1;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard2;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard3;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard4;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard5;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard6;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard7;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard8;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard9;

	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UInputAction> Keyboard0;

private:
	TWeakObjectPtr<ULetheWidgetController> ViewCardDetailWidgetController;
};
