// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheBasePlayerController.h"

#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"

ALetheBasePlayerController::ALetheBasePlayerController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALetheBasePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (const ULetheUIManagerSubsystem* UIManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<ULetheUIManagerSubsystem>())
	{
		UIManagerSubsystem->EnsureCreateRootLayout(GetLocalPlayer());
	}

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}
