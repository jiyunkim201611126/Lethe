// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheUserWidget.h"

void ULetheUserWidget::SetWidgetController(UObject* InWidgetController)
{
	WidgetController = InWidgetController;
	WidgetControllerSet();
}

void ULetheUserWidget::WidgetControllerSet_Implementation()
{
}
