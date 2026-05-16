// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingCardWidget.h"

#include "DeckEditingCardListObject.h"
#include "Components/Image.h"

void UDeckEditingCardWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	if (const UDeckEditingCardListObject* DeckEditingCardListObject = Cast<UDeckEditingCardListObject>(ListItemObject))
	{
		CardBorderImage->SetColorAndOpacity(DeckEditingCardListObject->CardTypeColor);
		CardImage->SetBrushFromTexture(DeckEditingCardListObject->CardTexture);
	}
}
