// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "AbilitySystemInterface.h"
#include "ActorSelectorComponent.h"
#include "PlayerAbilityRequestComponent.h"
#include "PreviewCoordinatorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
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

	PlayerAbilityRequestComponent = CreateDefaultSubobject<UPlayerAbilityRequestComponent>("PlayerAbilityRequestComponent");

	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

ULetheWidgetController* ALethePlayerController::InitPlayerUI(UAbilitySystemComponent* ASC, UAttributeSet* AS, UAttributeSet* PAS)
{
	LetheHUD->InitPlayerBattleUI(this, ASC, AS, PAS);
	return LetheHUD->CreatePlayerAttributeWidgetController(this, ASC, AS, PAS);
}

ULetheWidgetController* ALethePlayerController::InitEnemyUI(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	return LetheHUD->CreateEnemyAttributeWidgetController(this, ASC, AS);
}

void ALethePlayerController::OnWheeled(const float AttributeWidgetSize) const
{
	if (OnCameraHeightChangedDelegate.IsBound())
	{
		OnCameraHeightChangedDelegate.Broadcast(AttributeWidgetSize);
	}
}

void ALethePlayerController::HandleLeftMouseButtonClickedInWorldSection()
{
	if (SelectedCardAbility.IsValid())
	{
		// 선택된 카드가 있다면 얼리리턴합니다. 카드 사용은 RequestUseCard를 통해 이루어집니다.
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
	
	if (!SelectedCharacter.IsValid() && !OutTileAndActor.Actor)
	{
		// 캐릭터 미선택 상태에서 빈 타일을 클릭했다면 얼리리턴합니다.
		return;
	}

	if (SelectedCharacter.IsValid() && SelectedCharacter == OutTileAndActor.Actor)
	{
		// 선택했던 캐릭터가 서있던 타일을 선택한 경우(제자리 클릭) 들어오는 분기입니다.
		if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
		{
			PlayerAbilityRequestComponent->RemoveReservedMove(SelectedCharacter.Get());
			RefreshMovePreview();
		}
		ResetSelectedCharacter();
		return;
	}

	// SelectedCharacter가 null이라면 이번 클릭으로 캐릭터를 선택 중인 상황입니다.
	const bool bIsSelectingCharacter = !SelectedCharacter.IsValid();
	if (bIsSelectingCharacter)
	{
		if (OutTileAndActor.Actor->Implements<UPlayerCharacterInterface>())
		{
			SelectedCharacter = OutTileAndActor.Actor;
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
			PlayerAbilityRequestComponent->ReserveMove(SelectedCharacter.Get(), AbilitySystemComponent, OutTileAndActor.Tile);
			ResetSelectedCharacter();
			RefreshMovePreview();
		}
		break;
	case EPhaseState::PlayerTurnPhase:
		{
			// 이동 가능한 타일을 모두 가져옵니다.
			TArray<ATile*> OutMovableTiles;
			if (!PlayerAbilityRequestComponent->TryGetMovableTiles(SelectedCharacter.Get(), AbilitySystemComponent, OutMovableTiles))
			{
				ResetSelectedCharacter();
				break;
			}
		
			if (bIsSelectingCharacter)
			{
				// 이동 가능 범위를 하이라이팅합니다.
				ActorSelector->HighlightActorsByAbility(OutMovableTiles, SelectedCharacter.Get());
			}
			else
			{
				// 이동을 요청합니다.
				PlayerAbilityRequestComponent->RequestMove(SelectedCharacter.Get(), AbilitySystemComponent, OutMovableTiles, OutTileAndActor.Tile);
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
	bIsReservedMovePreviewingMode = !bIsReservedMovePreviewingMode;
	RefreshMovePreview();
}

void ALethePlayerController::RefreshMovePreview() const
{
	if (!bIsReservedMovePreviewingMode)
	{
		ArrowRenderer->DeactivateMovePreviewArrow();
		return;
	}
	
	TMap<APlayerCharacterBase*, TArray<FVector>> MovePathLocations;
	if (PlayerAbilityRequestComponent->TryGetMovePathLocations(MovePathLocations))
	{
		ArrowRenderer->DrawMovePreviewArrow(MovePathLocations);
	}
	else
	{
		ArrowRenderer->DeactivateMovePreviewArrow();
	}
}

void ALethePlayerController::StartResolvePlayerMoves() const
{
	PlayerAbilityRequestComponent->StartResolveMoves();
}

void ALethePlayerController::OnPlayerMovedResolved(AActor* MovedCharacter) const
{
	if (CurrentPhaseState == EPhaseState::PlayerMovePhase)
	{
		PlayerAbilityRequestComponent->OnPlayerReservedMoveResolved(MovedCharacter);
		RefreshMovePreview();
	}
}

void ALethePlayerController::OnSelectCardRequested(const int32 HandIndex, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
{
	if (!ArrowRenderer)
	{
		return;
	}

	// 기존에 선택된 카드가 있을 수 있으므로 먼저 선택을 취소합니다.
	ResetSelectedCard();
	
	if (OwnerASC && CardTag.IsValid())
	{
		// 카드와 캐릭터는 동시에 선택될 수 없으므로 캐릭터 선택은 초기화합니다.
		ResetSelectedCharacter();
		
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer CardTagContainer = CardTag.GetSingleTagContainer();
		OwnerASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(CardTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			return;
		}
		
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
					if (OnCancelCardSelectCancelDelegate.IsBound())
					{
						OnCancelCardSelectCancelDelegate.Broadcast();
					}
					return;
				}
				
				// 마우스 Hovered 시 Preview 구현을 위해 카드의 Ability를 캐싱해둡니다.
				SelectedCardAbility = LetheCardAbility;
				SelectedCardOwnerASC = OwnerASC;
				
				TArray<ATile*> OutTiles;
				ActorSelector->TryGetTilesByRangeFromActor(CardOwner, LetheCardAbility->GetAbilityRange(), ETileRangeQueryType::Any, OutTiles);
				ActorSelector->HighlightActorsByAbility(OutTiles, CardOwner);
			}
		}

		// 마우스를 타일 위에 올려둔 채로 카드를 키보드로 선택한 경우에도 타일 하이라이팅 등이 정상 작동할 수 있도록 명시적으로 한 번 호출합니다.
		FTileAndActor OutTileAndActor;
		ActorSelector->GetTileAndActorUnderCursor(OutTileAndActor);
		TArray<ATile*> OutTargetTiles;
		SelectedCardAbility->GetTargetTiles(SelectedCardOwnerASC->GetAvatarActor(), OutTileAndActor.Tile, OutTargetTiles);
		ActorSelector->HighlightTilesByMouse(OutTargetTiles, false);

		OnSelectCardDelegate.ExecuteIfBound(HandIndex);
	}
}

void ALethePlayerController::ResetSelectedCard()
{
	SelectedCardAbility.Reset();
	SelectedCardOwnerASC.Reset();
	ActorSelector->UnhighlightActorsByAbility();
	ActorSelector->UnhighlightActorByMouse();
	ArrowRenderer->DeactivateCardPreviewArrow();
	if (OnCancelCardSelectCancelDelegate.IsBound())
	{
		OnCancelCardSelectCancelDelegate.Broadcast();
	}
}

void ALethePlayerController::SetMouseOnWorldSection(const bool bInMouseOnWorldSection)
{
	bMouseOnWorldSection = bInMouseOnWorldSection;
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
		LetheGameState->OnPlayerMoveResolved.BindUObject(this, &ThisClass::OnPlayerMovedResolved);
		LetheGameState->OnCardUseResolved.BindUObject(this, &ThisClass::OnCardUseResolved);
	}
}

void ALethePlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangePhaseState.Remove(OnPhaseStateChangedHandle);
		LetheGameState->OnPlayerMoveResolved.Unbind();
		LetheGameState->OnCardUseResolved.Unbind();
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
		PlayerAbilityRequestComponent->SetAllReservedMovesWaitingForQueue();
		break;
	case EPhaseState::DrawPhase:
		// 전투 페이즈로 진입 시, 예약해뒀던 모든 이동을 초기화합니다.
		PlayerAbilityRequestComponent->ResetReservedMoveData();
		if (bIsReservedMovePreviewingMode)
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
	
	if (SelectedCharacter.IsValid() && bMouseOnWorldSection)
	{
		// 선택된 캐릭터가 있는 경우 들어오는 분기입니다.
		if (OutTileAndActor.Tile)
		{
			TArray<AActor*> HighlightTiles;
			HighlightTiles.Add(OutTileAndActor.Tile);
			ActorSelector->HighlightActorByMouse(HighlightTiles, false);
		}
		return;
	}

	if (SelectedCardAbility.IsValid() && SelectedCardOwnerASC.IsValid() && bMouseOnWorldSection)
	{
		// 선택된 카드가 있는 경우 들어오는 분기입니다.
		const ATile* CurrentTile = OutTileAndActor.Tile;
		const AActor* AvatarActor = SelectedCardOwnerASC->GetAvatarActor();
		
		if (CurrentTile && AvatarActor)
		{
			TArray<ATile*> OutAbilityRangeTiles;
			ActorSelector->TryGetTilesByRangeFromActor(AvatarActor, SelectedCardAbility->GetAbilityRange(), ETileRangeQueryType::Any, OutAbilityRangeTiles);
			if (OutAbilityRangeTiles.Contains(OutTileAndActor.Tile))
			{
				// 마우스를 올린 타일이 선택한 카드의 사용 범위 내에 있는 경우에만 하이라이팅을 활성화합니다.
				TArray<ATile*> OutTargetTiles;
				SelectedCardAbility->GetTargetTiles(AvatarActor, OutTileAndActor.Tile, OutTargetTiles);
				ActorSelector->HighlightTilesByMouse(OutTargetTiles, false);
				return;
			}

			// 사용 범위를 벗어난 경우 프리뷰 및 Arrow를 비활성화합니다.
			PreviewCoordinatorComponent->StopAllPreview();
			ArrowRenderer->DeactivateCardPreviewArrow();
		}
	}

	if (!SelectedCardAbility.IsValid() && !SelectedCharacter.IsValid() && bMouseOnWorldSection && CurrentPhaseState == EPhaseState::PlayerTurnPhase)
	{
		// 선택된 카드도 캐릭터도 없을 때, PlayerTurnPhase면 들어오는 분기입니다.
		// 이 경우 nullptr여도 이전 하이라이팅을 지워야 하기 때문에, null 체크 없이 호출합니다.
		TArray<AActor*> HighlightActors;
		HighlightActors.Add(OutTileAndActor.Actor);
		ActorSelector->HighlightActorByMouse(HighlightActors, true);
		return;
	}

	ActorSelector->UnhighlightActorByMouse();
}

void ALethePlayerController::OnOtherTileDetected(const TArray<AActor*>& CurrentActors) const
{
	if (!CurrentActors.IsEmpty())
	{
		if (SelectedCardOwnerASC.IsValid() && SelectedCardAbility.IsValid())
		{
			// 카드를 선택한 경우 들어오는 분기입니다.
			if (const AActor* SelectedCardOwnerActor = SelectedCardOwnerASC->GetAvatarActor())
			{
				FPreviewContext PreviewContext;
				PreviewContext.CurrentTargetActors.Append(CurrentActors);
				PreviewContext.SourceASC = SelectedCardOwnerASC.Get();
				PreviewContext.SelectedCardAbility = SelectedCardAbility.Get();
				PreviewCoordinatorComponent->StartCalculatingPreviewData(PreviewContext);
				ArrowRenderer->DrawCardPreviewArrow(SelectedCardOwnerActor, CurrentActors);
			}
		}
	}
	else
	{
		// 빈 타일에 마우스를 올린 경우 들어오는 분기입니다.
		PreviewCoordinatorComponent->StopAllPreview();
		ArrowRenderer->DeactivateCardPreviewArrow();
	}
}

void ALethePlayerController::OnUpdatePreviewData(const FPreviewData& PreviewData) const
{
	OnPreviewDataUpdatedDelegate.Broadcast(PreviewData);
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, const int32 InHandIndex)
{
	if (!PlayerAbilityRequestComponent->RequestUseCard(OwnerASC, SavedCard, InHandIndex))
	{
		OnResolveUseCardDelegate.ExecuteIfBound(InHandIndex, false);
	}
	// 사용 요청 결과에 관계 없이 선택했던 카드는 초기화합니다.
	ResetSelectedCard();
}

void ALethePlayerController::OnCardUseResolved(const int32 HandIndex, const bool bSuccess) const
{
	OnResolveUseCardDelegate.ExecuteIfBound(HandIndex, bSuccess);
}

void ALethePlayerController::GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, FText& OutText) const
{
	PlayerAbilityRequestComponent->GetCardDescriptionText(OwnerASC, SavedCard, OutText);
}

ULetheHUD* ALethePlayerController::GetLetheHUD() const
{
	return LetheHUD;
}

bool ALethePlayerController::IsCardSelected() const
{
	return SelectedCardAbility.IsValid();
}
