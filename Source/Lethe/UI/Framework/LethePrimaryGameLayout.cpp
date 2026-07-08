// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePrimaryGameLayout.h"

#include "Engine/GameInstance.h"
#include "Lethe/UI/Framework/LetheGameUIPolicy.h"
#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

ULethePrimaryGameLayout* ULethePrimaryGameLayout::GetPrimaryGameLayout(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return nullptr;
	}

	if (const UGameInstance* GameInstance = PlayerController->GetGameInstance())
	{
		if (ULetheUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<ULetheUIManagerSubsystem>())
		{
			if (ULetheGameUIPolicy* Policy = UIManager->GetCurrentUIPolicy())
			{
				return Policy->GetOrCreateRootLayout(PlayerController);
			}
		}
	}
	return nullptr;
}

void ULethePrimaryGameLayout::FindAndRemoveWidgetFromLayer(UCommonActivatableWidget* ActivatableWidget)
{
	if (!ActivatableWidget)
	{
		return;
	}

	for (const TPair<FGameplayTag, TObjectPtr<UCommonActivatableWidgetContainerBase>>& Layer : Layers)
	{
		Layer.Value->RemoveWidget(*ActivatableWidget);
	}
}

UCommonActivatableWidgetContainerBase* ULethePrimaryGameLayout::GetLayerWidget(const FGameplayTag LayerTag) const
{
	return Layers.FindRef(LayerTag);
}

void ULethePrimaryGameLayout::RegisterLayer(const FGameplayTag LayerTag, UCommonActivatableWidgetContainerBase* LayerWidget)
{
	if (!IsDesignTime() && ensure(LayerTag.IsValid()) && ensure(LayerWidget))
	{
		LayerWidget->OnTransitioningChanged.AddUObject(this, &ThisClass::OnWidgetStackTransitioning);
		LayerWidget->SetTransitionDuration(0.f);
		Layers.Add(LayerTag, LayerWidget);
	}
}

void ULethePrimaryGameLayout::OnWidgetStackTransitioning(UCommonActivatableWidgetContainerBase* Widget, bool bIsTransitioning)
{
}
