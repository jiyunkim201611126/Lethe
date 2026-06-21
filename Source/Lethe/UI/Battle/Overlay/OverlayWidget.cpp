// Copyright JETBLU, Inc. All Rights Reserved.

#include "OverlayWidget.h"

#include "Lethe/AbilitySystem/LetheAbilitySystemLibrary.h"
#include "Lethe/UI/Battle/Card/CardPanelWidget.h"
#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Lethe/UI/Framework/LetheActivatableWidget.h"
#include "Input/CommonUIInputTypes.h"

void UOverlayWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	RegisterUIActionBinding(FBindUIActionArgs(Keyboard1, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard1)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard2, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard2)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard3, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard3)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard4, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard4)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard5, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard5)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard6, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard6)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard7, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard7)));
	RegisterUIActionBinding(FBindUIActionArgs(Keyboard8, false, FSimpleDelegate::CreateUObject(this, &ThisClass::HandleKeyboard8)));
}

TOptional<FUIInputConfig> UOverlayWidget::GetDesiredInputConfig() const
{
	FUIInputConfig Config(ECommonInputMode::All, EMouseCaptureMode::CaptureDuringMouseDown, false);
	Config.bIgnoreMoveInput = false;
	Config.bIgnoreLookInput = false;

	return Config;
}

void UOverlayWidget::WidgetControllerSet_Implementation()
{
	CardPanel->SetWidgetController(ULetheAbilitySystemLibrary::GetCardPanelWidgetController(this));
}

void UOverlayWidget::HandleKeyboard1() const
{
	CardPanel->HandleKeyboardEvent(0);
}

void UOverlayWidget::HandleKeyboard2() const
{
	CardPanel->HandleKeyboardEvent(1);
}

void UOverlayWidget::HandleKeyboard3() const
{
	CardPanel->HandleKeyboardEvent(2);
}

void UOverlayWidget::HandleKeyboard4() const
{
	CardPanel->HandleKeyboardEvent(3);
}

void UOverlayWidget::HandleKeyboard5() const
{
	CardPanel->HandleKeyboardEvent(4);
}

void UOverlayWidget::HandleKeyboard6() const
{
	CardPanel->HandleKeyboardEvent(5);
}

void UOverlayWidget::HandleKeyboard7() const
{
	CardPanel->HandleKeyboardEvent(6);
}

void UOverlayWidget::HandleKeyboard8() const
{
	CardPanel->HandleKeyboardEvent(7);
}

void UOverlayWidget::HandleKeyboard9() const
{
	CardPanel->HandleKeyboardEvent(8);
}

void UOverlayWidget::HandleKeyboard0() const
{
	CardPanel->HandleKeyboardEvent(9);
}
