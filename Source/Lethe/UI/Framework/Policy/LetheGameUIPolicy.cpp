// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameUIPolicy.h"

#include "Framework/Application/SlateApplication.h"
#include "LetheGameUIFeature.h"
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"
#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"

ULetheGameUIPolicy* ULetheGameUIPolicy::GetGameUIPolicy(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}
	
	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (const UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (ULetheUIManagerSubsystem* UIManager = GameInstance->GetSubsystem<ULetheUIManagerSubsystem>())
			{
				return UIManager->GetCurrentUIPolicy();
			}
		}
	}
	return nullptr;
}

ULetheUIManagerSubsystem* ULetheGameUIPolicy::GetOwningUIManager() const
{
	return Cast<ULetheUIManagerSubsystem>(GetOuter());
}

ULethePrimaryGameLayout* ULetheGameUIPolicy::GetOrCreateRootLayout(APlayerController* PlayerController)
{
	if (!RootLayout)
	{
		CreateLayoutWidget(PlayerController);
	}
	return RootLayout;
}

ULethePrimaryGameLayout* ULetheGameUIPolicy::GetRootLayout() const
{
	return RootLayout;
}

void ULetheGameUIPolicy::AddLayoutToViewport(APlayerController* PlayerController, ULethePrimaryGameLayout* Layout)
{
	if (!PlayerController || !Layout)
	{
		return;
	}

	Layout->SetPlayerContext(FLocalPlayerContext(PlayerController));
	Layout->AddToPlayerScreen(1000);
	OnRootLayoutAddedToViewport(PlayerController, Layout);
}

void ULetheGameUIPolicy::OnRootLayoutAddedToViewport(APlayerController* PlayerController, ULethePrimaryGameLayout* Layout)
{
#if WITH_EDITOR
	if (GIsEditor && PlayerController)
	{
		FSlateApplication::Get().SetUserFocusToGameViewport(0);
	}
#endif
}

void ULetheGameUIPolicy::CreateLayoutWidget(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	const TSubclassOf<ULethePrimaryGameLayout> LoadedLayoutWidgetClass = LayoutWidgetClass.LoadSynchronous();
	if (ensure(LoadedLayoutWidgetClass && !LoadedLayoutWidgetClass->HasAnyClassFlags(CLASS_Abstract)))
	{
		RootLayout = CreateWidget<ULethePrimaryGameLayout>(PlayerController, LoadedLayoutWidgetClass);
		AddLayoutToViewport(PlayerController, RootLayout);

		for (ULetheGameUIFeature* UIFeature : UIFeatures)
		{
			if (UIFeature)
			{
				UIFeature->InitializeFeature(RootLayout);
			}
		}
	}
}

void ULetheGameUIPolicy::Deinitialize() const
{
	for (ULetheGameUIFeature* UIFeature : UIFeatures)
	{
		if (UIFeature)
		{
			UIFeature->DeinitializeFeature();
		}
	}
	
	if (RootLayout)
	{
		RootLayout->RemoveFromParent();
	}
}
