// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "AbilitySystemInterface.h"
#include "ActorSelectorComponent.h"
#include "PlayerAbilityRequestComponent.h"
#include "PreviewCoordinatorComponent.h"
#include "Engine/World.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Ability/LetheCardAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Actor/Tile/Tile.h"
#include "Lethe/Data/PreviewData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/HighlightInterface.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ALethePlayerController::ALethePlayerController()
{
	ActorSelector = CreateDefaultSubobject<UActorSelectorComponent>(TEXT("ActorSelector"));

	PreviewCoordinatorComponent = CreateDefaultSubobject<UPreviewCoordinatorComponent>(TEXT("PreviewCoordinatorComponent"));
	PreviewCoordinatorComponent->OnUpdatePreviewData.AddUObject(this, &ThisClass::OnUpdatePreviewData);

	PlayerAbilityRequestComponent = CreateDefaultSubobject<UPlayerAbilityRequestComponent>(TEXT("PlayerAbilityRequestComponent"));
}

void ALethePlayerController::BeginPlay()
{
	Super::BeginPlay();

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

	FTileHitResult OutTileHitResult;
	ActorSelector->GetTileHitResult(OutTileHitResult);
	if (!OutTileHitResult.Tile)
	{
		// Tile 검출에 실패했다면 얼리리턴합니다.
		ResetSelectedCharacter();
		return;
	}

	if (!SelectedCharacter.IsValid() && !OutTileHitResult.ActorOnTile)
	{
		// 캐릭터 미선택 상태에서 빈 타일을 클릭했다면 얼리리턴합니다.
		return;
	}

	if (SelectedCharacter.IsValid() && SelectedCharacter == OutTileHitResult.ActorOnTile)
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
		if (OutTileHitResult.ActorOnTile->Implements<UPlayerCharacterInterface>())
		{
			SelectedCharacter = OutTileHitResult.ActorOnTile;
		}
	}

	if (!SelectedCharacter.IsValid())
	{
		// 최종적으로 선택된 캐릭터가 없는 경우 얼리리턴합니다.
		return;
	}
	
	ActorSelector->SetHighlightedActors(EHighlightReason::SelectedCharacter, { SelectedCharacter.Get() });

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
			PlayerAbilityRequestComponent->ReserveMove(SelectedCharacter.Get(), AbilitySystemComponent, OutTileHitResult.Tile);
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
				// 선택된 캐릭터와 이동 가능 범위를 하이라이팅합니다.
				ActorSelector->SetHighlightedTiles(EHighlightReason::SelectCandidate, OutMovableTiles);
			}
			else
			{
				// 이동을 요청합니다.
				PlayerAbilityRequestComponent->RequestMove(SelectedCharacter.Get(), AbilitySystemComponent, OutMovableTiles, OutTileHitResult.Tile);
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
		ActorSelector->ClearHighlightedActors(EHighlightReason::SelectedCharacter);
		ActorSelector->ClearHighlightedActors(EHighlightReason::SelectCandidate);
		ActorSelector->ClearHighlightedActors(EHighlightReason::TargetCandidate);
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

void ALethePlayerController::OnSelectCardRequested(const int32 HandIndex, ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle)
{
	if (!ArrowRenderer)
	{
		return;
	}

	// 기존에 선택된 카드가 있을 수 있으므로 먼저 선택을 취소합니다.
	ResetSelectedCard();

	if (CardOwnerASC && AbilitySpecHandle.IsValid())
	{
		// 카드와 캐릭터는 동시에 선택될 수 없으므로 캐릭터 선택은 초기화합니다.
		ResetSelectedCharacter();

		const FGameplayAbilitySpec* AbilitySpec = CardOwnerASC->FindAbilitySpecFromHandle(AbilitySpecHandle);
		if (!AbilitySpec)
		{
			return;
		}

		// 선택된 카드의 범위에 해당하는 타일을 하이라이팅합니다.
		ULetheCardAbility* LetheCardAbility = Cast<ULetheCardAbility>(AbilitySpec->Ability);
		const AActor* CardOwner = CardOwnerASC->GetAvatarActor();
		if (LetheCardAbility && CardOwner)
		{
			if (CardOwnerASC->AbilityActorInfo.IsValid())
			{
				// TODO: 사용 못 할 경우 기준 필요함, 현재는 Cost 부족하면 바로 취소되도록 해놨음
				const FGameplayAbilityActorInfo* PreviewActorInfo = CardOwnerASC->AbilityActorInfo.Get();
				const bool bCanUse = LetheCardAbility->CheckCost(AbilitySpecHandle, PreviewActorInfo);
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
				SelectedCardOwnerASC = CardOwnerASC;

				TArray<ATile*> SourceTiles;
				GetTileUnderActorAsArray(GetWorld(), CardOwner, SourceTiles);
				ActorSelector->SetHighlightedTiles(EHighlightReason::Source, SourceTiles);
			}
		}

		OnSelectCardDelegate.ExecuteIfBound(HandIndex);
	}
}

void ALethePlayerController::ResetSelectedCard()
{
	SelectedCardAbility.Reset();
	SelectedCardOwnerASC.Reset();
	ActorSelector->ClearHighlightedActors(EHighlightReason::SelectCandidate);
	ActorSelector->ClearHighlightedActors(EHighlightReason::TargetCandidate);
	ActorSelector->ClearHighlightedActors(EHighlightReason::Source);
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

void ALethePlayerController::OnPhaseStateChanged(const EPhaseState OldPhaseState, const EPhaseState NewPhaseState)
{
	CurrentPhaseState = NewPhaseState;

	ResetSelectedCharacter();
	ResetSelectedCard();
	
	switch (CurrentPhaseState)
	{
	case EPhaseState::EnemyPlanPhase:
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

	FTileHitResult OutTileHitResult;
	ActorSelector->GetTileHitResult(OutTileHitResult);

	// 선택된 캐릭터가 있는 경우 들어가는 분기입니다.
	if (SelectedCharacter.IsValid())
	{
		// 마우스가 올려져있는 타일을 하이라이팅합니다.
		TArray<ATile*> MouseHoverTileArray;
		if (bMouseOnWorldSection && OutTileHitResult.Tile)
		{
			MouseHoverTileArray.Add(OutTileHitResult.Tile);
		}
		ActorSelector->SetHighlightedTiles(EHighlightReason::TargetCandidate, MouseHoverTileArray);
		return;
	}

	// 선택된 카드가 있는 경우 들어가는 분기입니다.
	if (SelectedCardAbility.IsValid() && SelectedCardOwnerASC.IsValid())
	{
		if (const AActor* CardOwner = SelectedCardOwnerASC->GetAvatarActor())
		{
			// 선택 가능한 타일과 타겟 후보 타일을 모두 가져옵니다.
			FEffectTargetTileSelectorContext Context;
			Context.AvatarActor = CardOwner;
			Context.TargetingIntent.HitTile = OutTileHitResult.Tile;
			Context.TargetingIntent.ImpactPoint = OutTileHitResult.ImpactPoint;
			SelectedCardAbility->GetCandidateTiles(Context);

			TArray<ATile*> TargetCandidateTiles;
			for (const FTargetSelectionResult& TargetResult : Context.OutTargetResults)
			{
				TargetCandidateTiles.Reserve(TargetCandidateTiles.Num() + TargetResult.Targets.Num());
				for (const FSelectedTarget& Target : TargetResult.Targets)
				{
					TargetCandidateTiles.Add(Target.TargetTile.Get());
				}
			}

			TArray<ATile*> SourceTiles;
			GetTileUnderActorAsArray(GetWorld(), CardOwner, SourceTiles);

			// 타겟 후보 타일을 성공적으로 검출해낸 경우 들어가는 분기입니다.
			if (!TargetCandidateTiles.IsEmpty() && bMouseOnWorldSection)
			{
				// 각각 역할에 맞게 하이라이팅합니다.
				ActorSelector->SetHighlightedTiles(EHighlightReason::SelectCandidate, Context.OutSelectCandidateTiles);
				ActorSelector->SetHighlightedTiles(EHighlightReason::Source, SourceTiles);
				const bool bTargetCandidateChanged = ActorSelector->SetHighlightedTiles(EHighlightReason::TargetCandidate, TargetCandidateTiles);
				if (bTargetCandidateChanged)
				{
					OnOtherTileDetected();
				}
				return;
			}

			// 사용 범위를 벗어난 경우 선택 후보만 하이라이팅하고 프리뷰 및 Arrow를 비활성화합니다.
			ActorSelector->SetHighlightedTiles(EHighlightReason::SelectCandidate, Context.OutSelectCandidateTiles);
			ActorSelector->SetHighlightedTiles(EHighlightReason::Source, SourceTiles);
			ActorSelector->ClearHighlightedActors(EHighlightReason::TargetCandidate);
			PreviewCoordinatorComponent->StopAllPreview();
			ArrowRenderer->DeactivateCardPreviewArrow();
			return;
		}
	}

	if (!SelectedCardAbility.IsValid() && !SelectedCharacter.IsValid() && bMouseOnWorldSection && CurrentPhaseState == EPhaseState::PlayerTurnPhase)
	{
		// 선택된 카드도 캐릭터도 없을 때, PlayerTurnPhase면 들어오는 분기입니다.
		ActorSelector->ClearHighlightedActors(EHighlightReason::TargetCandidate);
		return;
	}

	ActorSelector->ClearHighlightedActors(EHighlightReason::TargetCandidate);
}

void ALethePlayerController::OnOtherTileDetected() const
{
	PreviewCoordinatorComponent->StopAllPreview();
	ArrowRenderer->DeactivateCardPreviewArrow();

	if (SelectedCardOwnerASC.IsValid() && SelectedCardAbility.IsValid())
	{
		if (const AActor* SelectedCardOwnerActor = SelectedCardOwnerASC->GetAvatarActor())
		{
			FTileHitResult OutTileHitResult;
			ActorSelector->GetTileHitResult(OutTileHitResult);

			FEffectTargetTileSelectorContext Context;
			Context.AvatarActor = SelectedCardOwnerActor;
			Context.TargetingIntent.HitTile = OutTileHitResult.Tile;
			Context.TargetingIntent.ImpactPoint = OutTileHitResult.ImpactPoint;

			SelectedCardAbility->GetTargetTiles(Context);

			// Arrow 표시 용도의 TargetActors를 생성합니다.
			TArray<AActor*> TargetActors;
			for (const FTargetSelectionResult& TargetResult : Context.OutTargetResults)
			{
				TargetActors.Reserve(TargetActors.Num() + TargetResult.Targets.Num());
				for (const auto& Target : TargetResult.Targets)
				{
					if (Target.ActorOnTile.IsValid() && Target.ActorOnTile->Implements<UCombatInterface>())
					{
						TargetActors.Add(Target.ActorOnTile.Get());
					}
				}
			}

			FPreviewContext PreviewContext;
			PreviewContext.TargetSelectionResults = Context.OutTargetResults;
			PreviewContext.SourceASC = SelectedCardOwnerASC.Get();
			PreviewContext.SelectedCardAbility = SelectedCardAbility.Get();
			PreviewCoordinatorComponent->StartCalculatingPreviewData(PreviewContext);
			ArrowRenderer->DrawCardPreviewArrow(SelectedCardOwnerActor, TargetActors);
		}
	}
}

void ALethePlayerController::OnUpdatePreviewData(const FPreviewData& PreviewData) const
{
	OnPreviewDataUpdatedDelegate.Broadcast(PreviewData);
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const FGameplayTag& CardTag, const int32 InHandIndex)
{
	if (!PlayerAbilityRequestComponent->RequestUseCard(CardOwnerASC, AbilitySpecHandle, CardTag, InHandIndex))
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

bool ALethePlayerController::IsCardSelected() const
{
	return SelectedCardAbility.IsValid();
}

void ALethePlayerController::GetTileUnderActorAsArray(const UWorld* World, const AActor* Actor, TArray<ATile*>& OutTiles) const
{
	OutTiles.Reset();

	const UTileManagerSubsystem* TileManagerSubsystem = World ? World->GetSubsystem<UTileManagerSubsystem>() : nullptr;
	if (!TileManagerSubsystem)
	{
		return;
	}

	if (ATile* Tile = TileManagerSubsystem->GetTileUnderActor(Actor))
	{
		OutTiles.Add(Tile);
	}
}
