// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "TileSelectorComponent.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"

ALethePlayerController::ALethePlayerController()
{
	TileSelector = CreateDefaultSubobject<UTileSelectorComponent>("TileSelector");

	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void ALethePlayerController::OnNumberPressed(const int32 InNumber) const
{
	OnNumberKeyPressedDelegate.ExecuteIfBound(InNumber);
}

void ALethePlayerController::SetCardSelected(const bool bInCardSelected, const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
{
	if (!TileSelector)
	{
		return;
	}
	
	bCardSelected = bInCardSelected;
	if (bCardSelected && OwnerASC && CardTag.IsValid())
	{
		// CardTag에 해당하는 Ability를 가져옵니다.
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
		OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);

		// 선택된 카드의 범위에 해당하는 타일을 하이라이팅합니다.
		if (!AbilitySpecs.IsEmpty())
		{
			const ULetheGameplayAbility* LetheGameplayAbility = Cast<ULetheGameplayAbility>(AbilitySpecs[0]->Ability);
			const AActor* CardOwner = OwnerASC->GetOwner();
			if (LetheGameplayAbility && CardOwner)
			{
				TArray<ATile*> OutTiles;
				TileSelector->TryGetTilesByDepth(OutTiles, CardOwner, LetheGameplayAbility->GetAbilityRange());
				TileSelector->HighlightTileByCard(OutTiles);
			}
		}
	}
	else
	{
		TileSelector->UnhighlightTileByCard();
	}
}

void ALethePlayerController::SetMouseOnCardUseSection(const bool bInMouseOnCardUseSection)
{
	bMouseOnCardUseSection = bInMouseOnCardUseSection;
}

void ALethePlayerController::BeginPlay()
{
	Super::BeginPlay();

	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
}

void ALethePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnNumberKeyPressedDelegate.Unbind();
	Super::EndPlay(EndPlayReason);
}

void ALethePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	if (bCardSelected && bMouseOnCardUseSection)
	{
		// 카드 사용 준비가 완료된 경우 들어오는 분기입니다.
		FHitResult Hit;
		GetHitResultUnderCursor(ECC_Tile, false, Hit);
		if (Hit.IsValidBlockingHit())
		{
			// 마우스가 올라가있는 타일을 하이라이팅할 수 있도록 알려줍니다.
			TileSelector->HighlightTileByMouse(Hit.GetActor());
		}
		return;
	}

	TileSelector->UnhighlightTileByMouse();
}

bool ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
{
	SetCardSelected(false);

	if (OwnerASC)
	{
		if (AActor* TargetActor = TileSelector->GetActorOnTileUnderCursor())
		{
			// CardTag에 해당하는 Ability를 가져옵니다.
			TArray<FGameplayAbilitySpec*> AbilitySpecs;
			const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
			OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);

			// TODO: 중복 카드가 있다면 AbilitySpec이 여러 개 나오므로, 추후 CardLevel로 알맞은 Ability인지 확인하는 과정이 필요할 수 있습니다.
			// TODO: 현재는 첫번째 거로 사용합니다.
		
			if (!AbilitySpecs.IsEmpty())
			{
				// Ability가 발동될 수 있도록 이벤트를 발생시킵니다.
				FGameplayEventData Payload;
				Payload.Instigator = OwnerASC->GetAvatarActor();
				Payload.Target = TargetActor;

				// 카드 사용 성공 시 true를 반환합니다.
				return OwnerASC->TriggerAbilityFromGameplayEvent(AbilitySpecs[0]->Handle, OwnerASC->AbilityActorInfo.Get(), CardTag, &Payload, *OwnerASC);
			}
		}
	}
	
	return false;
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}
