// Copyright JETBLU, Inc. All Rights Reserved.

#include "LethePlayerController.h"

#include "AbilitySystemInterface.h"
#include "ActorSelectorComponent.h"
#include "PreviewCoordinatorComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/Abilities/LetheCardAbility.h"
#include "Lethe/Actor/ArrowRenderer/ArrowRenderer.h"
#include "Lethe/Data/PreviewData.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "Lethe/Manager/Tile/TileManagerSubsystem.h"

ALethePlayerController::ALethePlayerController()
{
	ActorSelector = CreateDefaultSubobject<UActorSelectorComponent>("ActorSelector");
	ActorSelector->OnDetectedOtherTile.BindUObject(this, &ThisClass::OnOtherTileDetected);

	PreviewCoordinatorComponent = CreateDefaultSubobject<UPreviewCoordinatorComponent>("PreviewCoordinatorComponent");
	PreviewCoordinatorComponent->OnUpdatePreviewData.AddUObject(this, &ThisClass::OnUpdatePreviewData);

	PrimaryActorTick.bCanEverTick = true;
	
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
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
	if (SelectedCardAbility.IsValid() || !ActorSelector)
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

	bool bIsSelectingCharacter = false;
	if ((!SelectedCharacter.IsValid() && !OutTileAndActor.Actor) || (SelectedCharacter.IsValid() && SelectedCharacter == OutTileAndActor.Actor))
	{
		// 캐릭터 미선택 상태에서 빈 타일을 클릭했거나, 이미 선택된 캐릭터와 동일한 캐릭터를 선택한 경우 얼리리턴합니다.
		return;
	}

	if (OutTileAndActor.Actor)
	{
		// 클릭한 타일에 무언가 있다면 일단 캐릭터 선택 상태를 초기화하고, 캐릭터 선택 로직을 시작합니다.
		ResetSelectedCharacter();
		if (OutTileAndActor.Actor->Implements<UPlayerCharacterInterface>())
		{
			SelectedCharacter = OutTileAndActor.Actor;
			bIsSelectingCharacter = true;
		}
	}

	if (!SelectedCharacter.IsValid())
	{
		// 최종적으로 선택된 캐릭터가 없는 경우 얼리리턴합니다.
		return;
	}
	
	const IAbilitySystemInterface* CurrentTargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(SelectedCharacter.Get());
	UAbilitySystemComponent* AbilitySystemComponent = CurrentTargetAbilitySystemInterface ? CurrentTargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
	
	if (AbilitySystemComponent)
	{
		// MoveAbility 사용 준비를 시작합니다.
		const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
		TArray<FGameplayAbilitySpec*> AbilitySpecs;
		const FGameplayTagContainer MoveTagContainer = LetheGameplayTags.Ability_Move.GetSingleTagContainer();
		AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(MoveTagContainer, AbilitySpecs);
		if (AbilitySpecs.IsEmpty())
		{
			ResetSelectedCharacter();
			return;
		}
		
		TArray<ATile*> OutTiles;
		if (const ULetheGameplayAbility* MoveAbility = Cast<ULetheGameplayAbility>(AbilitySpecs[0]->Ability))
		{
			ActorSelector->TryGetTilesByDepth(OutTiles, SelectedCharacter.Get(), MoveAbility->GetAbilityRange());
		}
		
		if (!OutTiles.IsEmpty())
		{
			if (bIsSelectingCharacter)
			{
				// 캐릭터를 선택해야 하는 경우 들어오는 분기입니다.
				ActorSelector->HighlightActorsByAbility(OutTiles, SelectedCharacter.Get());
			}
			else
			{
				// 이미 선택된 캐릭터가 있었고, 빈 타일을 클릭해 캐릭터를 이동시켜야 하는 경우 들어오는 분기입니다.
				switch (CurrentPhaseState)
				{
				case EPhaseState::PlayerMovePhase:
					ReserveMoveWhileNoneCombatPhase(OutTileAndActor.Tile);
					break;
				case EPhaseState::PlayerTurnPhase:
					FAbilityActivationData AbilityActivationData;
					AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
					AbilityActivationData.AbilityTag = LetheGameplayTags.Ability_Move;
					AbilityActivationData.AbilityOwnerASC = AbilitySystemComponent;
					AbilityActivationData.TargetTile = OutTileAndActor.Tile;
					TryMoveWhileCombatPhase(OutTiles, AbilityActivationData);
					break;
				default:
					return;
				}
				
				ResetSelectedCharacter();
			}
		}
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

void ALethePlayerController::ReserveMoveWhileNoneCombatPhase(const ATile* TargetTile) const
{
	ICombatInterface* CombatInterface = Cast<ICombatInterface>(SelectedCharacter);
	const UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>();
	if (CombatInterface && TileManagerSubsystem)
	{
		const int32 MoveDistance = CombatInterface->GetMoveDistance();
	}
}

void ALethePlayerController::TryMoveWhileCombatPhase(const TArray<ATile*>& TilesInRange, const FAbilityActivationData& AbilityActivationData) const
{
	if (UTileManagerSubsystem* TileManagerSubsystem = GetWorld()->GetSubsystem<UTileManagerSubsystem>())
	{
		if (TilesInRange.Contains(AbilityActivationData.TargetTile) && TileManagerSubsystem->CanMoveToTileForPlayerCharacter(AbilityActivationData.TargetTile.Get()))
		{
			// 선택한 타일로 이동 가능한 경우 들어오는 분기입니다.
			TileManagerSubsystem->ReservePlayerMoveTile(SelectedCharacter.Get(), AbilityActivationData.TargetTile.Get());
					
			if (const ALetheGameState* LetheGameState = Cast<ALetheGameState>(GetWorld()->GetGameState()))
			{
				LetheGameState->AddPlayerAbilityActivationData(AbilityActivationData);
			}
		}
	}
}

bool ALethePlayerController::SetCardSelected(const bool bInCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag)
{
	if (!ActorSelector || !ArrowRenderer)
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
	ArrowRenderer->SetActive(false);
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
	}
	Super::EndPlay(EndPlayReason);
}

void ALethePlayerController::OnPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState)
{
	CurrentPhaseState = NewState;
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
			const IAbilitySystemInterface* CurrentTargetAbilitySystemInterface = Cast<IAbilitySystemInterface>(CurrentActor);
			UAbilitySystemComponent* TargetASC = CurrentTargetAbilitySystemInterface ? CurrentTargetAbilitySystemInterface->GetAbilitySystemComponent() : nullptr;

			if (TargetASC)
			{
				FPreviewContext PreviewContext;
				PreviewContext.CurrentTargetASCs.Emplace(TargetASC);
				PreviewContext.SourceASC = SelectedCardOwnerASC.Get();
				PreviewContext.SelectedCardAbility = SelectedCardAbility.Get();
				PreviewCoordinatorComponent->StartCalculatingPreviewData(PreviewContext);
			}
			else
			{
				PreviewCoordinatorComponent->StopAllPreview();
			}
		}
	}
}

void ALethePlayerController::OnUpdatePreviewData(const FPreviewData& PreviewData) const
{
	OnPreviewDataUpdatedDelegate.Broadcast(PreviewData);
}

void ALethePlayerController::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag, const int32 InHandIndex) const
{
	if (ActorSelector && OwnerASC)
	{
		// 카드 사용 시엔 검출된 타일 위에 반드시 캐릭터가 있어야 합니다.
		FTileAndActor OutTileAndActor;
		ActorSelector->GetTileAndActorUnderCursor(OutTileAndActor);
		if (OutTileAndActor.Tile && OutTileAndActor.Actor)
		{
			// CardTag에 해당하는 CardAbilitySpec을 모두 가져옵니다.
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
					ActorSelector->TryGetTilesByDepth(OutTiles, OwnerASC->GetAvatarActor(), CardAbility->GetAbilityRange());
				
					// Ability 사용 범위 내의 타일을 선택한 경우 들어가는 분기입니다.
					if (OutTiles.Contains(OutTileAndActor.Tile))
					{
						// Ability가 사용될 수 있도록 이벤트 데이터를 생성합니다.
						FAbilityActivationData AbilityActivationData;
						AbilityActivationData.Index = InHandIndex;
						AbilityActivationData.AbilitySpecHandle = AbilitySpecs[0]->Handle;
						AbilityActivationData.AbilityTag = CardTag;
						AbilityActivationData.AbilityOwnerASC = OwnerASC;
						AbilityActivationData.TargetTile = OutTileAndActor.Tile;

						if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
						{
							// 카드 사용을 시작합니다.
							LetheGameState->AddPlayerAbilityActivationData(AbilityActivationData);
							return;
						}
					}
				}
			}
		}
	}

	// nullptr, CardTag에 해당하는 Ability 없음, 카드 사용 범위 바깥에 사용 시도 등 모종의 이유로 실패하면 이곳으로 내려옵니다.
	OnResolveUseCardDelegate.ExecuteIfBound(InHandIndex, false);
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

UPreviewCoordinatorComponent* ALethePlayerController::GetPreviewCoordinatorComponent() const
{
	return PreviewCoordinatorComponent;
}
