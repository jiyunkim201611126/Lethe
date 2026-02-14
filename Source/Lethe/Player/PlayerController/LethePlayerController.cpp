// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "TileSelectorComponent.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheGameplayAbility.h"

ALethePlayerController::ALethePlayerController()
{
	TileSelector = CreateDefaultSubobject<UTileSelectorComponent>("TileSelector");
	TileSelector->OnDetectedOtherTile.BindUObject(this, &ThisClass::OnOtherTileDetected);

	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	WaitingForUseCardsQueue.Reserve(MAX_HAND_COUNT - 1);
}

void ALethePlayerController::OnNumberPressed(const int32 InNumber) const
{
	OnNumberKeyPressedDelegate.ExecuteIfBound(InNumber);
}

void ALethePlayerController::SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
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
			ULetheGameplayAbility* LetheGameplayAbility = Cast<ULetheGameplayAbility>(AbilitySpecs[0]->Ability);
			const AActor* CardOwner = OwnerASC->GetOwner();
			if (LetheGameplayAbility && CardOwner)
			{
				if (OwnerASC->AbilityActorInfo.IsValid())
				{
					// TODO: 사용 못 할 경우 기준 필요함, 현재는 Cost 부족하면 바로 취소되도록 해놨음
					const FGameplayAbilityActorInfo* PreviewActorInfo = OwnerASC->AbilityActorInfo.Get();
					const bool bCanUse = LetheGameplayAbility->CheckCost(AbilitySpecs[0]->Handle, PreviewActorInfo);
					if (!bCanUse)
					{
						SetCardSelected(false);
						return;
					}
				
					// 마우스 Hovered 시 Preview 구현을 위해 카드의 Ability를 캐싱해둡니다.
					SelectedCardAbility = LetheGameplayAbility;
					SelectedCardOwnerASC = OwnerASC;
				
					TArray<ATile*> OutTiles;
					TileSelector->TryGetTilesByDepth(OutTiles, CardOwner, LetheGameplayAbility->GetAbilityRange());
					TileSelector->HighlightTileByCard(OutTiles, CardOwner);

					// AttributeWidgetController에게 카드가 선택되었음을 콜백으로 알려줍니다.
					OnCardSelectedDelegate.Broadcast(OwnerASC, LetheGameplayAbility);
				}
			}
		}
	}
	else
	{
		SelectedCardAbility = nullptr;
		SelectedCardOwnerASC = nullptr;
		TileSelector->UnhighlightTileByCard();
		TileSelector->UnhighlightTileByMouse();
		OnCancelCardSelectDelegate.Broadcast();
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

void ALethePlayerController::OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor) const
{
	if (SelectedCardOwnerASC.IsValid() && SelectedCardAbility.IsValid())
	{
		OnOtherTileDetectedDelegate.Broadcast(LastActor, CurrentActor, SelectedCardOwnerASC.Get(), SelectedCardAbility.Get());
	}
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex)
{
	// 이미 사용 대기 상태인 카드라면 선택하지 않고 얼리 리턴합니다.(이미 CardPanelWidget에서도 하고 있으나 한 번 더 방어 코드 작성)
	for (const FUseCardData& WaitingForUseCardData : WaitingForUseCardsQueue)
	{
		if (WaitingForUseCardData.HandIndex == InHandIndex)
		{
			return;
		}
	}
	
	AActor* TargetActor = TileSelector->GetActorOnTileUnderCursor();
	if (OwnerASC && TargetActor)
	{
		// CardTag에 해당하는 Ability를 가져옵니다.
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
		OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);

		// TODO: 중복 카드가 있다면 AbilitySpec이 여러 개 나오므로, 추후 CardLevel로 알맞은 Ability인지 확인하는 과정이 필요할 수 있습니다.
		// TODO: 현재는 첫번째 거로 사용합니다.
		
		if (!AbilitySpecs.IsEmpty())
		{
			// Ability가 발동될 수 있도록 이벤트 데이터를 생성합니다.
			FGameplayEventData Payload;
			Payload.Instigator = OwnerASC->GetAvatarActor();
			Payload.Target = TargetActor;

			// Queue에 넣고 Ability 발동을 시작합니다.
			FUseCardData UseCardData;
			UseCardData.HandIndex = InHandIndex;
			UseCardData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
			UseCardData.CardTag = CardTag;
			UseCardData.Payload = Payload;
			UseCardData.AbilityOwnerASC = OwnerASC;

			WaitingForUseCardsQueue.Emplace(UseCardData);
			if (!bIsProgressingCardAbility)
			{
				TryUseNextCardAbility();
			}
		}
	}
	else
	{
		OnResolveUseCardDelegate.ExecuteIfBound(InHandIndex, false);
	}
}

void ALethePlayerController::TryUseNextCardAbility()
{
	if (!WaitingForUseCardsQueue.IsEmpty())
	{
		const FUseCardData& NextCardData = WaitingForUseCardsQueue[0];
		
		const bool bUseSuccess = NextCardData.AbilityOwnerASC->TriggerAbilityFromGameplayEvent(NextCardData.AbilitySpecHandle, NextCardData.AbilityOwnerASC->AbilityActorInfo.Get(), NextCardData.CardTag, &NextCardData.Payload, *NextCardData.AbilityOwnerASC);
		bIsProgressingCardAbility = bUseSuccess;

		// Queue에서 사용 시도한 카드를 제거하고 성공 여부를 콜백으로 알려줍니다.
		OnResolveUseCardDelegate.ExecuteIfBound(NextCardData.HandIndex, bUseSuccess);
		WaitingForUseCardsQueue.RemoveAt(0);
		
		if (!bUseSuccess)
		{
			// 카드 사용 실패 시 Queue에 있는 모든 카드 사용 데이터를 실패로 간주하고, 이를 Widget에게 전달합니다.
			for (const FUseCardData& WaitingCardData : WaitingForUseCardsQueue)
			{
				OnResolveUseCardDelegate.ExecuteIfBound(WaitingCardData.HandIndex, false);
			}
			WaitingForUseCardsQueue.Reset();
		}
	}
	else
	{
		bIsProgressingCardAbility = false;
	}
}

void ALethePlayerController::OnAbilityEnded()
{
	TryUseNextCardAbility();
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}

bool ALethePlayerController::IsProgressingCardAbility() const
{
	return bIsProgressingCardAbility;
}
