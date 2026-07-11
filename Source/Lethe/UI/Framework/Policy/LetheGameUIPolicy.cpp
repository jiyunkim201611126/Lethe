// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheGameUIPolicy.h"

#include "Framework/Application/SlateApplication.h"
#include "LetheGameUIFeature.h"
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
		// RootLayout이 유효하지 않다면 Feature의 Initialize 호출이 필요하기 때문에 여기서 false로 변경합니다.
		bFeaturesInitialized = false;
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

	// Feature를 통해 Layout에 기본적으로 추가될 위젯들을 구성합니다.
	if (IsValid(RootLayout) && !bFeaturesInitialized)
	{
		for (ULetheGameUIFeature* UIFeature : UIFeatures)
		{
			if (UIFeature)
			{
				UIFeature->InitializeFeature(RootLayout);
			}
		}
		bFeaturesInitialized = true;
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

void ULetheGameUIPolicy::DeinitializeFeatures()
{
	for (ULetheGameUIFeature* UIFeature : UIFeatures)
	{
		if (UIFeature)
		{
			UIFeature->DeinitializeFeature();
		}
	}
	bFeaturesInitialized = false;
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
