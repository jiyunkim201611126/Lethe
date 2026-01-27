// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "Lethe/Lethe.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Lethe/UI/Widget/Card/CardWidget.h"

ALethePlayerController::ALethePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALethePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ALethePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bReadyToUseCard)
	{
		// 카드 사용 준비 상태일 경우 들어오는 분기입니다.
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Tile, false, Hit);

		if (Hit.IsValidBlockingHit())
		{
			LastActor = ThisActor;
			ThisActor = Hit.GetActor();

			if (LastActor != ThisActor)
			{
				if (LastActor)
				{
					LastActor->UnHighlightActor();
				}
				if (ThisActor)
				{
					ThisActor->HighlightActor();
				}
			}
		}

		return;
	}
	
	if (LastActor)
	{
		LastActor->UnHighlightActor();
		LastActor = nullptr;
	}
	if (ThisActor)
	{
		ThisActor->UnHighlightActor();
		ThisActor = nullptr;
	}
}

void ALethePlayerController::SetReadyToUseCard(const bool bReady)
{
	bReadyToUseCard = bReady;
}

bool ALethePlayerController::RequestUseCard(const UCardWidget* InCardWidget)
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Tile, false, Hit);

	SetReadyToUseCard(false);

	if (Hit.IsValidBlockingHit())
	{
		return true;
	}
	
	return false;
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}
