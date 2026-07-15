// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameUIPolicy.h"

#include "Framework/Application/SlateApplication.h"
#include "Blueprint/GameViewportSubsystem.h"
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

ULethePrimaryGameLayout* ULetheGameUIPolicy::GetOrCreateRootLayout(ULocalPlayer* LocalPlayer)
{
	if (!IsValid(RootLayout))
	{
		CreateLayoutWidget(LocalPlayer);
	}
	return RootLayout;
}

ULethePrimaryGameLayout* ULetheGameUIPolicy::GetRootLayout() const
{
	return RootLayout;
}

void ULetheGameUIPolicy::CreateLayoutWidget(ULocalPlayer* LocalPlayer)
{
	if (!LocalPlayer)
	{
		return;
	}

	// RootLayout의 유효성을 확인하고, 없다면 생성합니다.
	if (!IsValid(RootLayout))
	{
		RootLayout = nullptr;
		
		const TSubclassOf<ULethePrimaryGameLayout> LoadedLayoutWidgetClass = RootLayoutWidgetClass.LoadSynchronous();
		if (ensure(LoadedLayoutWidgetClass && !LoadedLayoutWidgetClass->HasAnyClassFlags(CLASS_Abstract)))
		{
			if (APlayerController* PlayerController = LocalPlayer->GetPlayerController(GetWorld()))
			{
				RootLayout = CreateWidget<ULethePrimaryGameLayout>(PlayerController, LoadedLayoutWidgetClass);
			}
		}
	}

	// RootLayout 유효성과 별개로 Viewport에 추가됐는지 확인합니다.
	if (IsValid(RootLayout))
	{
		const UGameViewportSubsystem* ViewportSubsystem = UGameViewportSubsystem::Get(GetWorld());
		if (!ViewportSubsystem || !ViewportSubsystem->IsWidgetAdded(RootLayout))
		{
			AddLayoutToViewport(LocalPlayer, RootLayout);
		}
	}

	if (IsValid(RootLayout))
	{
		for (const auto& StartUIFeatureClass : StartUIFeatureClasses)
		{
			const TSubclassOf<ULetheGameUIFeature> LoadedUIFeatureClass = StartUIFeatureClass.LoadSynchronous();
			
			if (ULetheGameUIFeature* CreatedFeature = NewObject<ULetheGameUIFeature>(this, LoadedUIFeatureClass))
			{
				CreatedFeature->InitializeFeature(GetRootLayout());
				UIFeatures.Add(CreatedFeature);
			}
		}
	}
}

void ULetheGameUIPolicy::AddLayoutToViewport(ULocalPlayer* LocalPlayer, ULethePrimaryGameLayout* Layout)
{
	if (!LocalPlayer || !Layout)
	{
		return;
	}

	Layout->SetPlayerContext(FLocalPlayerContext(LocalPlayer));
	Layout->AddToPlayerScreen(1000);
	OnRootLayoutAddedToViewport(LocalPlayer, Layout);
}

void ULetheGameUIPolicy::OnRootLayoutAddedToViewport(ULocalPlayer* LocalPlayer, ULethePrimaryGameLayout* Layout)
{
#if WITH_EDITOR
	if (GIsEditor && LocalPlayer)
	{
		FSlateApplication::Get().SetUserFocusToGameViewport(0);
	}
#endif
}

void ULetheGameUIPolicy::Deinitialize()
{
	for (ULetheGameUIFeature* UIFeature : UIFeatures)
	{
		if (UIFeature)
		{
			UIFeature->DeinitializeFeature();
		}
	}
	UIFeatures.Empty();
	
	if (RootLayout)
	{
		RootLayout->RemoveFromParent();
	}
}

UWorld* ULetheGameUIPolicy::GetWorld() const
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return nullptr;
	}

	if (const ULetheUIManagerSubsystem* UIManagerSubsystem = GetOwningUIManager())
	{
		return UIManagerSubsystem->GetWorld();
	}

	return nullptr;
}
