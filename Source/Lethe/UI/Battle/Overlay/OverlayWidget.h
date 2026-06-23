// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheActivatableWidget.h"
#include "OverlayWidget.generated.h"

class UCardPanelWidget;
class UInputAction;

UCLASS()
class LETHE_API UOverlayWidget : public ULetheActivatableWidget
{
	GENERATED_BODY()

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeOnInitialized() override;
	//~ End of UUserWidget Interface

	//~ Begin UCommonActivatableWidget Interface
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
	//~ End of UCommonActivatableWidget Interface
	
	//~ Begin ULetheActivatableWidget Interface
	virtual void WidgetControllerSet_Implementation() override;
	//~ End of ULetheActivatableWidget Interface

private:
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
};
