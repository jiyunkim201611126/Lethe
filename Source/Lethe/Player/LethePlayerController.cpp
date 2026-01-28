// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Lethe/UI/Widget/Card/CardWidget.h"

ALethePlayerController::ALethePlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALethePlayerController::OnNumberPressed(const int32 InNumber) const
{
	OnNumberKeyPressedDelegate.ExecuteIfBound(InNumber);
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
	// 우선 카드 드래그가 중단될 수 있도록 제어합니다.
	SetReadyToUseCard(false);
	
	// 커서 아래로 라인 트레이스를 통해 타일 검출을 시도합니다.
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Tile, false, Hit);

	if (Hit.IsValidBlockingHit())
	{
		// 타일 검출에 성공한 경우 들어오는 분기입니다.
		ULetheAbilitySystemComponent* OwnerASC = InCardWidget->GetOwnerASC();
		const ATile* Tile = Cast<ATile>(Hit.GetActor());
		
		if (OwnerASC && Tile)
		{
			// CardTag를 통해 발동할 Ability를 가져옵니다.
			TArray<FGameplayAbilitySpec*> AbilitySpec;
			const FGameplayTagContainer CardTag = InCardWidget->GetCardTag().GetSingleTagContainer();
			OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTag, AbilitySpec);

			// TODO: 중복 카드가 있다면 AbilitySpec이 여러 개 나오므로, 추후 CardLevel로 알맞은 Ability인지 확인하는 과정이 필요할 수 있습니다.
			// TODO: 현재는 첫번째 거로 사용합니다.

			// 타일 위에 있는 대상을 가져옵니다.
			const AActor* TargetActor = Tile->GetActorOnTile<AActor>();
			
			if (!AbilitySpec.IsEmpty() && TargetActor)
			{
				// Ability가 발동될 수 있도록 이벤트를 발생시킵니다.
				FGameplayEventData Payload;
				Payload.Instigator = OwnerASC->GetAvatarActor();
				Payload.Target = TargetActor;

				// 카드 사용 성공 시 true를 반환합니다.
				return OwnerASC->TriggerAbilityFromGameplayEvent(AbilitySpec[0]->Handle, OwnerASC->AbilityActorInfo.Get(), InCardWidget->GetCardTag(), &Payload, *OwnerASC);
			}
		}
	}
	
	return false;
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}
