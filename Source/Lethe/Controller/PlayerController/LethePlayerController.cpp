// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "PreviewCoordinatorComponent.h"
#include "TileSelectorComponent.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Game/LetheGameState.h"
#include "Lethe/Interface/PlayableCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/TileManagerSubsystem.h"

ALethePlayerController::ALethePlayerController()
{
	TileSelector = CreateDefaultSubobject<UTileSelectorComponent>("TileSelector");
	TileSelector->OnDetectedOtherTile.BindUObject(this, &ThisClass::OnOtherTileDetected);

	PreviewCoordinatorComponent = CreateDefaultSubobject<UPreviewCoordinatorComponent>("PreviewCoordinatorComponent");

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

void ALethePlayerController::OnWheeled(const float AttributeWidgetSize) const
{
	if (OnCameraHeightChangedDelegate.IsBound())
	{
		OnCameraHeightChangedDelegate.Broadcast(AttributeWidgetSize);
	}
}

void ALethePlayerController::OnLeftMouseButtonClickedOnWorld()
{
	if (SelectedCardAbility.IsValid() || !TileSelector)
	{
		return;
	}

	ALetheGameState* GameState = Cast<ALetheGameState>(GetWorld()->GetGameState());
	if (!GameState || GameState->GetTurnPhase() != EPhaseState::PlayerTurnPhase)
	{
		return;
	}

	FTileAndActor TileAndActor;
	TileSelector->GetTileAndActorUnderCursor(TileAndActor);
	if (!TileAndActor.Tile)
	{
		// Tile 검출에 실패했다면 얼리리턴합니다.
		ResetSelectedCharacter();
		return;
	}

	bool bIsSelectingCharacter = false;
	if ((!SelectedCharacter.IsValid() && !TileAndActor.Actor) || (SelectedCharacter.IsValid() && SelectedCharacter == TileAndActor.Actor))
	{
		// 캐릭터 미선택 상태에서 빈 타일을 클릭했거나, 이미 선택된 캐릭터와 동일한 캐릭터를 선택한 경우 얼리리턴합니다.
		return;
	}

	if (TileAndActor.Actor)
	{
		// 클릭한 타일에 무언가 있다면 일단 캐릭터 선택 상태를 초기화하고, 캐릭터 선택 로직을 시작합니다.
		ResetSelectedCharacter();
		if (TileAndActor.Actor->Implements<UPlayableCharacterInterface>())
		{
			SelectedCharacter = TileAndActor.Actor;
			bIsSelectingCharacter = true;
		}
	}

	if (!SelectedCharacter.IsValid())
	{
		// 최종적으로 선택된 캐릭터가 없는 경우 얼리리턴합니다.
		return;
	}

	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	TArray<FGameplayAbilitySpec*> AbilitySpecs;
	const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
	if (UAbilitySystemComponent* AbilitySystemComponent = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SelectedCharacter.Get()))
	{
		AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			ResetSelectedCharacter();
			return;
		}
		
		TArray<ATile*> OutTiles;
		if (const ULetheGameplayAbility* MoveAbility = Cast<ULetheGameplayAbility>(AbilitySpecs[0]->Ability))
		{
			TileSelector->TryGetTilesByDepth(OutTiles, SelectedCharacter.Get(), MoveAbility->GetAbilityRange());
		}
		
		if (!OutTiles.IsEmpty())
		{
			if (bIsSelectingCharacter)
			{
				// 캐릭터를 선택한 경우 들어오는 분기입니다.
				TileSelector->HighlightTileByAbility(OutTiles, SelectedCharacter.Get());
			}
			else
			{
				// 캐릭터를 이동시켜야 하는 경우 들어오는 분기입니다.
				if (OutTiles.Contains(TileAndActor.Tile))
				{
					// 선택한 타일로 이동 가능한 경우 들어오는 분기입니다.
					FGameplayEventData Payload;
					Payload.Instigator = SelectedCharacter.Get();
					Payload.OptionalObject = TileAndActor.Tile;
					AbilitySystemComponent->TriggerAbilityFromGameplayEvent(AbilitySpecs[0]->Handle, AbilitySystemComponent->AbilityActorInfo.Get(), LetheGameplayTags.Ability_Move, &Payload, *AbilitySystemComponent);
				}
				ResetSelectedCharacter();
			}
		}
	}
}

void ALethePlayerController::ResetSelectedCharacter()
{
	SelectedCharacter.Reset();
	TileSelector->UnhighlightTileByAbility();
}

bool ALethePlayerController::SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
{
	if (!TileSelector || !ArrowRenderer)
	{
		return false;
	}
	
	if (bInCardSelected && OwnerASC && CardTag.IsValid())
	{
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
		OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			return false;
		}
		
		ResetSelectedCharacter();
		
		// 선택된 카드의 범위에 해당하는 타일을 하이라이팅합니다.
		ULetheCardAbility* LetheCardAbility = Cast<ULetheCardAbility>(AbilitySpecs[0]->Ability);
		const AActor* CardOwner = OwnerASC->GetOwner();
		if (LetheCardAbility && CardOwner)
		{
			if (OwnerASC->AbilityActorInfo.IsValid())
			{
				// TODO: 사용 못 할 경우 기준 필요함, 현재는 Cost 부족하면 바로 취소되도록 해놨음
				const FGameplayAbilityActorInfo* PreviewActorInfo = OwnerASC->AbilityActorInfo.Get();
				const bool bCanUse = LetheCardAbility->CheckCost(AbilitySpecs[0]->Handle, PreviewActorInfo);
				if (!bCanUse)
				{
					return SetCardSelected(false);
				}
				
				// 마우스 Hovered 시 Preview 구현을 위해 카드의 Ability를 캐싱해둡니다.
				SelectedCardAbility = LetheCardAbility;
				SelectedCardOwnerASC = OwnerASC;
				
				TArray<ATile*> OutTiles;
				TileSelector->TryGetTilesByDepth(OutTiles, CardOwner, LetheCardAbility->GetAbilityRange());
				TileSelector->HighlightTileByAbility(OutTiles, CardOwner);
			}
		}

		OnOtherTileDetected(nullptr, TileSelector->GetActorOnTileUnderCursor());
		
		return true;
	}
	
	SelectedCardAbility = nullptr;
	SelectedCardOwnerASC = nullptr;
	TileSelector->UnhighlightTileByAbility();
	TileSelector->UnhighlightTileByMouse();
	ArrowRenderer->SetActive(false);
	if (OnCancelCardSelectDelegate.IsBound())
	{
		OnCancelCardSelectDelegate.Broadcast();
	}
	return false;
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

	check(ArrowRendererClass);
	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ArrowRenderer = GetWorld()->SpawnActor<AArrowRenderer>(ArrowRendererClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParameters);
}

void ALethePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnNumberKeyPressedDelegate.Unbind();
	Super::EndPlay(EndPlayReason);
}

void ALethePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	if ((SelectedCardAbility.IsValid() || SelectedCharacter.IsValid()) && bMouseOnCardUseSection)
	{
		// 선택된 카드가 있거나 선택된 캐릭터가 있는 경우 들어오는 분기입니다.
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
		const AActor* SelectedCardOwnerActor = SelectedCardOwnerASC->GetAvatarActor();
		if (SelectedCardOwnerActor && CurrentActor)
		{
			ArrowRenderer->SetPoints(SelectedCardOwnerActor, CurrentActor);
		}
		else
		{
			ArrowRenderer->SetActive(false);
		}

		if (PreviewCoordinatorComponent)
		{
			FPreviewContext PreviewContext;
			PreviewContext.CurrentTargetActors.Emplace(CurrentActor);
			PreviewContext.SourceASC = SelectedCardOwnerASC.Get();
			PreviewContext.SelectedCardAbility = SelectedCardAbility.Get();
			PreviewCoordinatorComponent->StartCalculatingPreviewData(PreviewContext);
		}
	}
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex)
{
	// 이미 사용 대기 상태인 카드라면 아무 동작도 하지 않고 얼리 리턴합니다.(이미 CardPanelWidget에서도 하고 있으나 한 번 더 방어 코드 작성)
	for (const FUseCardData& WaitingForUseCardData : WaitingForUseCardsQueue)
	{
		if (WaitingForUseCardData.HandIndex == InHandIndex)
		{
			return;
		}
	}

	if (!TileSelector)
	{
		OnResolveUseCardDelegate.ExecuteIfBound(InHandIndex, false);
		return;
	}

	FTileAndActor TileAndActor;
	TileSelector->GetTileAndActorUnderCursor(TileAndActor);
	if (OwnerASC && TileAndActor.Actor)
	{
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
		OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);

		// TODO: 중복 카드가 있다면 AbilitySpec이 여러 개 나오므로, 추후 CardLevel로 알맞은 Ability인지 확인하는 과정이 필요할 수 있습니다.
		// TODO: 현재는 첫번째 거로 사용합니다.
		
		if (!AbilitySpecs.IsEmpty())
		{
			if (const ULetheCardAbility* CardAbility = Cast<ULetheCardAbility>(AbilitySpecs[0]->Ability))
			{
				TArray<ATile*> OutTiles;
				TileSelector->TryGetTilesByDepth(OutTiles, OwnerASC->GetOwner(), CardAbility->GetAbilityRange());
				if (!OutTiles.Contains(TileAndActor.Tile))
				{
					OnResolveUseCardDelegate.ExecuteIfBound(InHandIndex, false);
					return;
				}
				
				// Ability가 발동될 수 있도록 이벤트 데이터를 생성합니다.
				FGameplayEventData Payload;
				Payload.Instigator = OwnerASC->GetAvatarActor();
				Payload.Target = TileAndActor.Actor;

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

void ALethePlayerController::OnAbilityEnded(const bool bUseSuccess)
{
	if (bUseSuccess)
	{
		TryUseNextCardAbility();
	}
	else
	{
		for (const FUseCardData& WaitingCardData : WaitingForUseCardsQueue)
		{
			OnResolveUseCardDelegate.ExecuteIfBound(WaitingCardData.HandIndex, false);
		}
		WaitingForUseCardsQueue.Reset();
	}
}

void ALethePlayerController::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const
{
	TArray<FGameplayAbilitySpecHandle> OutAbilityHandles;
	OwnerASC->GetAllAbilities(OutAbilityHandles);
	for (const FGameplayAbilitySpecHandle& Handle : OutAbilityHandles)
	{
		const FGameplayAbilitySpec* Spec = OwnerASC->FindAbilitySpecFromHandle(Handle);
		if (!Spec || !Spec->Ability)
		{
			continue;
		}

		if (const ULetheCardAbility* CardAbility = Cast<ULetheCardAbility>(Spec->Ability))
		{
			if (CardAbility->GetAssetTags().HasAllExact(CardTag.GetSingleTagContainer()))
			{
				OutText = CardAbility->GetCardDescription(OwnerASC, 1);
				return;
			}
		}
	}
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}

bool ALethePlayerController::IsProgressingCardAbility() const
{
	return bIsProgressingCardAbility;
}

UPreviewCoordinatorComponent* ALethePlayerController::GetPreviewCoordinatorComponent() const
{
	return PreviewCoordinatorComponent;
}
