// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheActivatableWidget.h"

void ULetheActivatableWidget::SetWidgetController(ULetheWidgetController* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}

void ULetheActivatableWidget::WidgetControllerSet_Implementation()
{
}
