// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

ALethePlayerController::ALethePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALethePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}
