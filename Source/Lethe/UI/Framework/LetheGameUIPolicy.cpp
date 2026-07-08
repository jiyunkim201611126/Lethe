// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameUIPolicy.h"

#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Framework/Application/SlateApplication.h"
#include "Lethe/UI/Framework/LethePrimaryGameLayout.h"
#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"

ULetheGameUIPolicy* ULetheGameUIPolicy::GetGameUIPolicy(const UObject* WorldContextObject)
{
	if (const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
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

UWorld* ULetheGameUIPolicy::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (const ULetheUIManagerSubsystem* UIManager = GetOwningUIManager())
	{
		return UIManager->GetGameInstance()->GetWorld();
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

	const TSubclassOf<ULethePrimaryGameLayout> LayoutWidgetClass = GetLayoutWidgetClass();
	if (ensure(LayoutWidgetClass && !LayoutWidgetClass->HasAnyClassFlags(CLASS_Abstract)))
	{
		RootLayout = CreateWidget<ULethePrimaryGameLayout>(PlayerController, LayoutWidgetClass);
		AddLayoutToViewport(PlayerController, RootLayout);
	}
}

TSubclassOf<ULethePrimaryGameLayout> ULetheGameUIPolicy::GetLayoutWidgetClass() const
{
	return LayoutClass.LoadSynchronous();
}
