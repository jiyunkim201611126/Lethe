// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePrimaryGameLayout.h"

#include "Engine/GameInstance.h"
#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

ULethePrimaryGameLayout* ULethePrimaryGameLayout::GetPrimaryGameLayout(const APlayerController* PlayerController)
{
	return PlayerController ? GetPrimaryGameLayout(PlayerController->GetLocalPlayer()) : nullptr;
}

ULethePrimaryGameLayout* ULethePrimaryGameLayout::GetPrimaryGameLayout(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return nullptr;
	}

	if (const UGameInstance* GameInstance = LocalPlayer->GetGameInstance())
	{
		if (ULetheUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<ULetheUIManagerSubsystem>())
		{
			if (ULetheGameUIPolicy* Policy = UIManager->GetCurrentUIPolicy())
			{
				return Policy->GetOrCreateRootLayout(LocalPlayer);
			}
		}
	}
	return nullptr;
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
