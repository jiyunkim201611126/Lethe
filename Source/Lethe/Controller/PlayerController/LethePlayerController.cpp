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
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

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
		const AActor* CardOwner = OwnerASC->GetAvatarActor();
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
			}
		}

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
	if (ActorSelector)
	{
		ActorSelector->OnDetectedOtherTile.Unbind();
	}
	
	if (PreviewCoordinatorComponent)
	{
		PreviewCoordinatorComponent->OnUpdatePreviewData.RemoveAll(this);
	}
	
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
			TArray<ATile*> HighlightTile;
			HighlightTile.Add(OutTileAndActor.Tile);
			ActorSelector->HighlightTilesByMouse(HighlightTile);
		}
		return;
	}

	// 선택된 카드가 있는 경우 들어가는 분기입니다.
	if (SelectedCardAbility.IsValid() && SelectedCardOwnerASC.IsValid() && bMouseOnWorldSection)
	{
		const ATile* CurrentTile = OutTileAndActor.Tile;
		AActor* CardOwner = SelectedCardOwnerASC->GetAvatarActor();
		
		if (CurrentTile && CardOwner)
		{
			// 선택 가능한 타일과 타겟 후보 타일을 모두 가져옵니다.
			TArray<ATile*> OutSelectCandidateTiles;
			TArray<ATile*> OutTargetCandidateTiles;
			SelectedCardAbility->GetCandidateTiles(CardOwner, this, OutSelectCandidateTiles, OutTargetCandidateTiles);
			
			// 타겟 후보 타일을 성공적으로 검출해낸 경우 들어가는 분기입니다.
			if (!OutTargetCandidateTiles.IsEmpty())
			{
				// 선택 가능 타일 중 타겟 후보 타일을 제거합니다.
				OutSelectCandidateTiles.RemoveAll([&OutTargetCandidateTiles](const ATile* Tile)
				{
					return OutTargetCandidateTiles.Contains(Tile);
				});

				// 각각 역할에 맞게 하이라이팅합니다.
				ActorSelector->HighlightActorsByAbility(OutSelectCandidateTiles, CardOwner);
				ActorSelector->HighlightTilesByMouse(OutTargetCandidateTiles);
				return;
			}

			// 사용 범위를 벗어난 경우 선택 후보만 하이라이팅하고 프리뷰 및 Arrow를 비활성화합니다.
			ActorSelector->HighlightActorsByAbility(OutSelectCandidateTiles, CardOwner);
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
		ActorSelector->HighlightActorsByMouse(HighlightActors);
		return;
	}

	ActorSelector->UnhighlightActorByMouse();
}

void ALethePlayerController::OnOtherTileDetected() const
{
	PreviewCoordinatorComponent->StopAllPreview();
	ArrowRenderer->DeactivateCardPreviewArrow();

	if (SelectedCardOwnerASC.IsValid() && SelectedCardAbility.IsValid())
	{
		if (const AActor* SelectedCardOwnerActor = SelectedCardOwnerASC->GetAvatarActor())
		{
			TArray<ATile*> OutTargetTiles;
			SelectedCardAbility->GetTargetTiles(SelectedCardOwnerActor, this, OutTargetTiles);

			if (const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
			{
				TArray<AActor*> TargetActors;
				for (const ATile* Tile : OutTargetTiles)
				{
					if (AActor* TargetActor = TileManagerSubsystem->GetActorOnTile(Tile))
					{
						if (TargetActor->Implements<UCombatInterface>())
						{
							TargetActors.Add(TargetActor);
							continue;
						}
					}
					// EffectTargetMappingPolicies에서 TargetActors의 인덱스를 기반으로 로직을 수행하기 때문에, nullptr도 추가해야 합니다.
					TargetActors.Add(nullptr);
				}
				
				FPreviewContext PreviewContext;
				PreviewContext.CurrentTargetActors.Append(TargetActors);
				PreviewContext.SourceASC = SelectedCardOwnerASC.Get();
				PreviewContext.SelectedCardAbility = SelectedCardAbility.Get();
				PreviewCoordinatorComponent->StartCalculatingPreviewData(PreviewContext);
				ArrowRenderer->DrawCardPreviewArrow(SelectedCardOwnerActor, TargetActors);
			}
		}
	}
}

void ALethePlayerController::OnUpdatePreviewData(const FPreviewData& PreviewData) const
{
	OnPreviewDataUpdatedDelegate.Broadcast(PreviewData);
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, const int32 InHandIndex)
{
	if (!PlayerAbilityRequestComponent->RequestUseCard(this, OwnerASC, SavedCard, InHandIndex))
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

bool ALethePlayerController::IsCardSelected() const
{
	return SelectedCardAbility.IsValid();
}
