// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "AbilitySystemInterface.h"
#include "ActorSelectorComponent.h"
#include "PlayerAbilityContextComponent.h"
#include "PreviewCoordinatorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Data/PreviewData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/UI/Framework/LetheHUD.h"

ALethePlayerController::ALethePlayerController()
{
	ActorSelector = CreateDefaultSubobject<UActorSelectorComponent>("ActorSelector");
	ActorSelector->OnDetectedOtherTile.BindUObject(this, &ThisClass::OnOtherTileDetected);

	PreviewCoordinatorComponent = CreateDefaultSubobject<UPreviewCoordinatorComponent>("PreviewCoordinatorComponent");
	PreviewCoordinatorComponent->OnUpdatePreviewData.AddUObject(this, &ThisClass::OnUpdatePreviewData);

	PlayerAbilityContextComponent = CreateDefaultSubobject<UPlayerAbilityContextComponent>("PlayerAbilityContextComponent");

	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

ULetheWidgetController* ALethePlayerController::InitPlayerUI(APlayerState* PS, UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	LetheHUD->InitPlayerBattleUI(this, PS, ASC, AS);
	return LetheHUD->CreatePlayerAttributeWidgetController(this, PS, ASC, AS);
}

ULetheWidgetController* ALethePlayerController::InitEnemyUI(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	return LetheHUD->CreateEnemyAttributeWidgetController(this, ASC, AS);
}

void ALethePlayerController::OnNumberKeyPressed(const int32 InNumber) const
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
	if (SelectedCardAbility.IsValid())
	{
		// 선택된 카드가 있다면 얼리리턴합니다.
		return;
	}
	
	if (CurrentPhaseState != EPhaseState::PlayerMovePhase && CurrentPhaseState != EPhaseState::PlayerTurnPhase)
	{
		// 플레이어의 턴이 아니라면 얼리리턴합니다.
		return;
	}

	FTileAndActor OutTileAndActor;
	ActorSelector->GetTileAndActorUnderCursor(OutTileAndActor);
	if (!OutTileAndActor.Tile)
	{
		// Tile 검출에 실패했다면 얼리리턴합니다.
		ResetSelectedCharacter();
		return;
	}
	
	if ((!SelectedCharacter.IsValid() && !OutTileAndActor.Actor) || (SelectedCharacter.IsValid() && SelectedCharacter == OutTileAndActor.Actor))
	{
		// 캐릭터 미선택 상태에서 빈 타일을 클릭했거나, 이미 선택된 캐릭터와 동일한 캐릭터를 선택한 경우 얼리리턴합니다.
		return;
	}

	bool bIsSelectingCharacter = false;
	if (OutTileAndActor.Actor)
	{
		// 클릭한 타일에 무언가 있다면 일단 캐릭터 선택 상태를 초기화하고, 캐릭터 선택 로직을 시작합니다.
		ResetSelectedCharacter();
		if (OutTileAndActor.Actor->Implements<UPlayerCharacterInterface>())
		{
			// 캐릭터를 캐싱하고, '이번 입력으로 캐릭터를 선택 중임'을 기록합니다.
			SelectedCharacter = OutTileAndActor.Actor;
			bIsSelectingCharacter = true;
		}
	}

	if (!SelectedCharacter.IsValid())
	{
		// 최종적으로 선택된 캐릭터가 없는 경우 얼리리턴합니다.
		return;
	}
	
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(SelectedCharacter);
	UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	if (!AbilitySystemComponent)
	{
		ResetSelectedCharacter();
		return;
	}

	switch (CurrentPhaseState)
	{
	case EPhaseState::PlayerMovePhase:
		if (!bIsSelectingCharacter)
		{
			// 이동 타일을 예약합니다.
			PlayerAbilityContextComponent->ReserveMove(SelectedCharacter.Get(), AbilitySystemComponent, OutTileAndActor.Tile);
			ResetSelectedCharacter();
			RefreshMovePreview();
		}
		break;
	case EPhaseState::PlayerTurnPhase:
		{
			// 이동 가능한 타일을 모두 가져옵니다.
			TArray<ATile*> OutTiles;
			if (!PlayerAbilityContextComponent->TryGetMovableTiles(SelectedCharacter.Get(), AbilitySystemComponent, OutTiles))
			{
				ResetSelectedCharacter();
				break;
			}
		
			if (bIsSelectingCharacter)
			{
				// 이동 가능 범위를 하이라이팅합니다.
				ActorSelector->HighlightActorsByAbility(OutTiles, SelectedCharacter.Get());
			}
			else
			{
				// 이동을 요청합니다.
				PlayerAbilityContextComponent->RequestMove(SelectedCharacter.Get(), AbilitySystemComponent, OutTiles, OutTileAndActor.Tile);
				ResetSelectedCharacter();
			}
		}
		break;
	default:
		ResetSelectedCharacter();
		break;
	}
}

void ALethePlayerController::ResetSelectedCharacter()
{
	if (SelectedCharacter.IsValid())
	{
		ActorSelector->UnhighlightActorsByAbility();
		SelectedCharacter.Reset();
	}
}

void ALethePlayerController::ToggleMovePreview()
{
	bIsReservedMovePreviewingMove = !bIsReservedMovePreviewingMove;
	RefreshMovePreview();
}

void ALethePlayerController::RefreshMovePreview() const
{
	if (!bIsReservedMovePreviewingMove)
	{
		ArrowRenderer->DeactivateArrow();
		return;
	}
	
	TMap<APlayerCharacterBase*, TArray<FVector>> MovePathLocations;
	if (PlayerAbilityContextComponent->TryGetMovePathLocations(MovePathLocations))
	{
		ArrowRenderer->DrawMovePreviewArrow(MovePathLocations);
	}
	else
	{
		ArrowRenderer->DeactivateArrow();
	}
}

void ALethePlayerController::StartResolvePlayerMoves() const
{
	PlayerAbilityContextComponent->StartResolveMoves();
}

void ALethePlayerController::OnPlayerMovedResolved(AActor* MovedCharacter) const
{
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		PlayerAbilityContextComponent->OnPlayerMoveResolved(MovedCharacter);
		RefreshMovePreview();
	}
}

bool ALethePlayerController::SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
{
	if (!ArrowRenderer)
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
		AActor* CardOwner = OwnerASC->GetAvatarActor();
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
				ActorSelector->TryGetTilesByDepth(OutTiles, CardOwner, LetheCardAbility->GetAbilityRange());
				ActorSelector->HighlightActorsByAbility(OutTiles, CardOwner);
			}
		}

		// 마우스를 타일 위에 올려둔 채로 카드를 키보드로 선택한 경우에도 타일 하이라이팅 등이 정상 작동할 수 있도록 명시적으로 한 번 호출합니다.
		FTileAndActor OutTileAndActor;
		ActorSelector->GetTileAndActorUnderCursor(OutTileAndActor);
		OnOtherTileDetected(nullptr, OutTileAndActor.Actor);
		return true;
	}
	
	SelectedCardAbility = nullptr;
	SelectedCardOwnerASC = nullptr;
	ActorSelector->UnhighlightActorsByAbility();
	ActorSelector->UnhighlightActorByMouse();
	ArrowRenderer->DeactivateArrow();
	if (OnCancelCardSelectCancelDelegate.IsBound())
	{
		OnCancelCardSelectCancelDelegate.Broadcast();
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

	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		OnPhaseStateChangedHandle = LetheGameState->OnChangePhaseState.AddUObject(this, &ThisClass::OnPhaseStateChanged);
		
		if (UAbilityResolverComponent* AbilityResolverComponent = LetheGameState->GetAbilityResolverComponent())
		{
			AbilityResolverComponent->OnCardUseResolved.BindWeakLambda(this,
				[this](const int32 HandIndex, const bool bSuccess)
				{
					OnResolveUseCardDelegate.ExecuteIfBound(HandIndex, bSuccess);
				});
		}
		
		LetheGameState->OnPlayerMoveResolved.BindUObject(this, &ThisClass::OnPlayerMovedResolved);
	}
}

void ALethePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnNumberKeyPressedDelegate.Unbind();
	
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangePhaseState.Remove(OnPhaseStateChangedHandle);
		
		if (UAbilityResolverComponent* AbilityResolverComponent = LetheGameState->GetAbilityResolverComponent())
		{
			AbilityResolverComponent->OnCardUseResolved.Unbind();
		}

		LetheGameState->OnPlayerMoveResolved.Unbind();
	}
	Super::EndPlay(EndPlayReason);
}

void ALethePlayerController::OnPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState)
{
	CurrentPhaseState = NewState;

	switch (CurrentPhaseState)
	{
	case EPhaseState::EnemyPlanningPhase:
		// 비전투 페이즈로 진입 시, 예약해뒀던 모든 이동이 큐에 들어갈 수 있도록 상태를 활성화합니다.
		PlayerAbilityContextComponent->SetAllReservedMovesWaitingForQueue();
		break;
	case EPhaseState::DrawPhase:
		// 전투 페이즈로 진입 시, 예약해뒀던 모든 이동을 초기화합니다.
		PlayerAbilityContextComponent->ResetReservedMoveData();
		if (bIsReservedMovePreviewingMove)
		{
			ToggleMovePreview();
		}
		break;
	default:
		break;
	}
}

void ALethePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	FTileAndActor OutTileAndActor;
	ActorSelector->GetTileAndActorUnderCursor(OutTileAndActor);
	
	if ((SelectedCardAbility.IsValid() || SelectedCharacter.IsValid()) && bMouseOnCardUseSection)
	{
		// 선택된 카드가 있거나 선택된 캐릭터가 있는 경우 들어오는 분기입니다.
		if (OutTileAndActor.Tile)
		{
			ActorSelector->HighlightActorByMouse(OutTileAndActor.Tile, false);
		}
		return;
	}

	if (!SelectedCardAbility.IsValid() && !SelectedCharacter.IsValid() && bMouseOnCardUseSection && CurrentPhaseState == EPhaseState::PlayerTurnPhase)
	{
		// 선택된 카드도 캐릭터도 없을 때, PlayerTurnPhase면 들어오는 분기입니다.
		// 이 경우 nullptr여도 이전 하이라이팅을 지워야 하기 때문에, null 체크 없이 호출합니다.
		ActorSelector->HighlightActorByMouse(OutTileAndActor.Actor, true);
		return;
	}

	ActorSelector->UnhighlightActorByMouse();
}

void ALethePlayerController::OnOtherTileDetected(AActor* LastActor, AActor* CurrentActor) const
{
	if (CurrentActor)
	{
		if (SelectedCardOwnerASC.IsValid() && SelectedCardAbility.IsValid())
		{
			// 카드를 선택한 경우 들어오는 분기입니다.
			if (const AActor* SelectedCardOwnerActor = SelectedCardOwnerASC->GetAvatarActor())
			{
				FPreviewContext PreviewContext;
				PreviewContext.CurrentTargetActors.Emplace(CurrentActor);
				PreviewContext.SourceASC = SelectedCardOwnerASC.Get();
				PreviewContext.SelectedCardAbility = SelectedCardAbility.Get();
				PreviewCoordinatorComponent->StartCalculatingPreviewData(PreviewContext);
				ArrowRenderer->DrawSkillPreviewArrow(SelectedCardOwnerActor, CurrentActor);
			}
		}
	}
	else
	{
		// 빈 타일에 마우스를 올린 경우 들어오는 분기입니다.
		PreviewCoordinatorComponent->StopAllPreview();
		ArrowRenderer->DeactivateArrow();
	}
}

void ALethePlayerController::OnUpdatePreviewData(const FPreviewData& PreviewData) const
{
	OnPreviewDataUpdatedDelegate.Broadcast(PreviewData);
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex) const
{
	if (!PlayerAbilityContextComponent->RequestUseCard(OwnerASC, CardTag, InHandIndex))
	{
		OnResolveUseCardDelegate.ExecuteIfBound(InHandIndex, false);
	}
}

void ALethePlayerController::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, FText& OutText) const
{
	PlayerAbilityContextComponent->GetCardDescriptionText(OwnerASC, CardTag, OutText);
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}
