// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "Lethe/Lethe.h"

ALethePlayerController::ALethePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

bool ALethePlayerController::RequestUseCard(const FGameplayTag& InCardTag) const
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Tile, false, Hit);

	if (Hit.IsValidBlockingHit())
	{
		return true;
	}
	
	return FMath::RandBool();
}

void ALethePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeUIOnly InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
}
