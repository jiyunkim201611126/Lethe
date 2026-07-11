// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardWidget.h"

#include "Components/InvalidationBox.h"
#include "Lethe/UI/Battle/DeckEditing/CardWidgetInitContext.h"
#include "Lethe/UI/Core/LetheImage.h"

void UCardWidget::InitCardView(const UCardWidgetInitContext* InContext) const
{
	CardImage->SetBrushFromTexture(InContext->CardTexture);
	TypeFrameImage->SetColorAndOpacity(InContext->CardTypeColor);
	InvalidationBox->SetCanCache(false);
}

void UCardWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	if (const UCardWidgetInitContext* InitContext = Cast<UCardWidgetInitContext>(ListItemObject))
	{
		InitCardView(InitContext);
	}
}
