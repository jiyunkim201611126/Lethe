// Copyright JETBLU, Inc. All Rights Reserved.

#include "DeckEditingUIFeature.h"

#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/UI/Battle/DeckEditing/DeckEditingWidget.h"
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"

void UDeckEditingUIFeature::OnInitialized()
{
	if (!DeckEditingWidget && LayoutWidget.IsValid())
	{
		const TSubclassOf<UDeckEditingWidget> LoadedWidgetClass = DeckEditingWidgetClass.LoadSynchronous();
		if (ensure(LoadedWidgetClass))
		{
			DeckEditingWidget = LayoutWidget->PushWidgetToLayerStack<UDeckEditingWidget>(FLetheGameplayTags::Get().UI_Layer_Game, LoadedWidgetClass);
		}
	}
}

void UDeckEditingUIFeature::OnDeinitialized()
{
	if (DeckEditingWidget)
	{
		DeckEditingWidget->DeactivateWidget();
		DeckEditingWidget->RemoveFromParent();
		DeckEditingWidget = nullptr;
	}
}
