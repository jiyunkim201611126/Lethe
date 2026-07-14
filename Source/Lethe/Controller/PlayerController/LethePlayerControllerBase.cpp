// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerControllerBase.h"

#include "Lethe/UI/Framework/LetheUIManagerSubsystem.h"

ALethePlayerControllerBase::ALethePlayerControllerBase()
{
	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALethePlayerControllerBase::BeginPlay()
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
