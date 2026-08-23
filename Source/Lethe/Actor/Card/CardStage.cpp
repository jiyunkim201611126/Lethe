// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardStage.h"

#include "CardActor.h"
#include "CardContainerManager.h"
#include "DeckBoxes.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Components/BoxComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/FX/FXManagerSubsystem.h"

ACardStage::ACardStage()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>("Root");
	SetRootComponent(Root);
	
	CaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("CaptureComponent"));
	CaptureComponent->SetupAttachment(Root);
	CaptureComponent->ProjectionType = ECameraProjectionMode::Type::Orthographic;
	CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	CaptureComponent->ShowFlags.SetAtmosphere(false);
	CaptureComponent->ShowFlags.SetFog(false);
	CaptureComponent->ShowFlags.SetVolumetricFog(false);
	CaptureComponent->ShowFlags.SetCloud(false);
	CaptureComponent->CaptureSource = SCS_SceneColorHDR;
}

void ACardStage::BeginPlay()
{
	Super::BeginPlay();

	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangeTurnPhaseState.AddUObject(this, &ThisClass::OnTurnPhaseStateChanged);
	}

	CardContainerManager = NewObject<UCardContainerManager>(this);
	DeckBoxes = GetWorld()->SpawnActor<ADeckBoxes>(DeckBoxesClass);
	DeckBoxes->SetActorTransform(GetActorTransform());
	
	UseRequestedCards.Reserve(MAX_HAND_SLOT_COUNT);

	CaptureComponent->ShowOnlyActors.Add(DeckBoxes);
}

void ACardStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangeTurnPhaseState.RemoveAll(this);
	}
	
	OnViewCardDetailRequested.Unbind();
	OnSelectCardRequested.Unbind();
	OnDrawPhaseCompleted.Unbind();
	OnStartResolvePlayerMovesRequested.Unbind();
	OnTurnEndRequested.Unbind();
	OnUseCardRequested.Unbind();

	for (ACardActor* SpawnedCard : SpawnedCards)
	{
		if (SpawnedCard)
		{
			SpawnedCard->OnCardMouseEventDelegate.Unbind();
			SpawnedCard->Destroy();
		}
	}
	SpawnedCards.Reset();

	CardContainerManager = nullptr;
	DeckBoxes = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ACardStage::Initialize(const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents)
{
	if (!CardContainerManager || !DeckBoxes)
	{
		return;
	}

	// 모든 캐릭터가 넘어올 때까지 계속해서 ASC를 캐싱, CardContainerManager에게 넘겨줍니다.
	AbilitySystemComponents.Reset();
	AbilitySystemComponents.Reserve(InAbilitySystemComponents.Num());
	for (ULetheAbilitySystemComponent* ASC : InAbilitySystemComponents)
	{
		AbilitySystemComponents.Add(ASC);
	}

	CardContainerManager->Initialize(AbilitySystemComponents, DeckBoxes);

	if (bInitialized)
	{
		return;
	}
	bInitialized = true;

	if (CaptureComponent->TextureTarget)
	{
		CaptureComponent->TextureTarget->ClearColor = FLinearColor::Transparent;
	}
}

void ACardStage::CreateCard(const FCardInitParams& CardInitParams)
{
	if (!CardContainerManager || !CardActorClass)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	if (ACardActor* CreatedCard = GetWorld()->SpawnActor<ACardActor>(CardActorClass, GetActorTransform(), SpawnParams))
	{
		CreatedCard->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
		CreatedCard->SetCardInfo(CardInitParams);
		CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);
		SpawnedCards.Add(CreatedCard);

		CardContainerManager->AddCardToDeck(CreatedCard);
		if (CardContainerManager->AreAllDecksFull())
		{
			CardContainerManager->ShuffleDeck();
			UpdateAllCardLocations();
		}

		CaptureComponent->ShowOnlyActors.Add(CreatedCard);
	}
}

bool ACardStage::HandleLeftMouseButtonClickedInCardStageSection(const FVector2D& TargetUV)
{
	if (ACardActor* ClickedCard = GetCardActorAtUV(TargetUV))
	{
		ClickedCard->HandleCardMouseEvent(ECardMouseEvent::LeftMouseButtonUp);
		return true;
	}

	if (UBoxComponent* ClickedDeckBox = GetDeckBoxCollisionAtUV(TargetUV))
	{
		const int32 ClickedDeckIndex = DeckBoxes->GetDeckIndex(ClickedDeckBox);
		if (ClickedDeckIndex == INDEX_NONE || !AbilitySystemComponents.IsValidIndex(ClickedDeckIndex))
		{
			return false;
		}

		ULetheAbilitySystemComponent* ClickedDeckOwnerASC = AbilitySystemComponents[ClickedDeckIndex].Get();
		
		if (CurrentTurnPhaseState == ETurnPhaseState::DrawPhase)
		{
			TryDraw(ClickedDeckOwnerASC);
		}
		else if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
		{
			// 비전투 중 덱 박스를 클릭했다면, 안에 있는 모든 카드를 밖으로 꺼내 보여줍니다.
			CardContainerManager->StopPreviewDeck(false);
			DeckBoxes->SetOpenReason(ClickedDeckBox, EDeckBoxOpenReason::Pinned, true);
			CardContainerManager->PreviewDeck(ClickedDeckOwnerASC);
		}

		CurrentSelectedDeckBox = ClickedDeckBox;
		return true;
	}
	return false;
}

void ACardStage::HandleCapturedMouseMove(const FVector2D& TargetUV) const
{
	// 커서 아래에 DeckBox가 있고, 현재 비전투 페이즈라면 해당 DeckBox를 엽니다.
	DeckBoxes->SetOpenReason(GetDeckBoxCollisionAtUV(TargetUV), EDeckBoxOpenReason::MouseHover, true);
}

void ACardStage::HandleCapturedMouseLeave() const
{
	DeckBoxes->SetOpenReason(nullptr, EDeckBoxOpenReason::MouseHover, false);
}

bool ACardStage::HandleLeftMouseButtonClickedInWorldSection()
{
	if (!CurrentSelectedCard || !CardContainerManager || !OnUseCardRequested.IsBound() || !OnSelectCardRequested.IsBound())
	{
		return false;
	}

	const int32 HandSlotIndex = CardContainerManager->FindCurrentHandSlotIndex(CurrentSelectedCard);
	if (HandSlotIndex == INDEX_NONE)
	{
		return false;
	}

	// 사용 요청된 카드임을 기록하고 콜백 함수를 호출합니다.
	UseRequestedCards.Add(HandSlotIndex, CurrentSelectedCard);
	OnUseCardRequested.Execute(CurrentSelectedCard->GetOwnerASC(), CurrentSelectedCard->GetAbilitySpecHandle(), CurrentSelectedCard->GetSavedCard().CardTag, HandSlotIndex);
	return true;
}

void ACardStage::HandleKeyboardEvent(const int32 Number) const
{
	switch (CurrentTurnPhaseState)
	{
	case ETurnPhaseState::DrawPhase:
		OnKeyboardEventWhenDrawPhase(Number);
		break;
	case ETurnPhaseState::PlayerTurnPhase:
	case ETurnPhaseState::PlayerMovePhase:
		OnKeyboardEventWhenPlayerPhase(Number);
		break;
	default:
		break;
	}
}

void ACardStage::OnTurnPhaseStateChanged(const ETurnPhaseState OldTurnPhaseState, const ETurnPhaseState NewTurnPhaseState)
{
	if (OldTurnPhaseState == ETurnPhaseState::PlayerTurnPhase)
	{
		if (CardContainerManager)
		{
			CardContainerManager->AddAllHandSlotsToGraveyard();
		}
		UpdateAllCardLocations();

		if (TurnEndSoundTag.IsValid())
		{
			if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
			{
				FXManagerSubsystem->AsyncPlaySound2D(TurnEndSoundTag, 1.f, 1.f);
			}
		}
	}
	
	CurrentTurnPhaseState = NewTurnPhaseState;

	if (CurrentTurnPhaseState == ETurnPhaseState::DrawPhase)
	{
		OnDrawPhaseStarted();
	}

	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		if (LetheGameState->IsBattlePhase())
		{
			// 전투 중인 상황이므로, 덱을 펼쳐보던 것을 중단하고 모든 덱 박스를 엽니다.
			DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Pinned, false);
			CardContainerManager->StopPreviewDeck();
			CurrentSelectedDeckBox.Reset();
			
			DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Battle, true);
		}
		else
		{
			// 모든 카드를 덱으로 되돌린 후 덱 박스를 닫습니다.
			CardContainerManager->StopPreviewDeck();
			CurrentSelectedDeckBox.Reset();
			
			CardContainerManager->AddAllHandSlotsToGraveyard();
			CardContainerManager->RefillDeck();
			CardContainerManager->ShuffleDeck();
			UpdateAllCardLocations();
			
			DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Battle, false);
		}
	}
}

void ACardStage::OnCardSelected(const int32 HandSlotIndex)
{
	const TArray<FHandSlot>& HandSlots = CardContainerManager->GetCurrentHandSlots();
	if (HandSlots.IsValidIndex(HandSlotIndex))
	{
		CurrentSelectedCard = HandSlots[HandSlotIndex].GetCard();
		if (CurrentSelectedCard)
		{
			CurrentSelectedCard->SetCardContainer(ECardContainer::Selected);
		}
	}
}

void ACardStage::OnCancelSelectedCard()
{
	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->SetCardContainer(ECardContainer::Hands);
		CurrentSelectedCard = nullptr;
	}
}

void ACardStage::OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess)
{
	ACardActor* CardActor = UseRequestedCards.FindRef(HandSlotIndex);
	if (!CardActor)
	{
		return;
	}

	if (CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
	{
		CardActor->SetCardContainer(ECardContainer::Deck);
	}
	else if (CurrentTurnPhaseState == ETurnPhaseState::PlayerTurnPhase)
	{
		if (bSuccess)
		{
			if (CardContainerManager)
			{
				CardContainerManager->AddCardToGraveyard(CardActor);
			}
			UpdateAllCardLocations();
		}
		else
		{
			CardActor->SetCardContainer(ECardContainer::Hands);
		}
	}

	UseRequestedCards.Remove(HandSlotIndex);
}

bool ACardStage::TryViewDetail(const FVector2D& TargetUV) const
{
	if (ACardActor* CardActor = GetCardActorAtUV(TargetUV))
	{
		CardActor->HandleCardMouseEvent(ECardMouseEvent::RightMouseButtonUp);
		return true;
	}
	return false;
}

void ACardStage::OnTurnEndButtonClicked() const
{
	switch (CurrentTurnPhaseState)
	{
	case ETurnPhaseState::PlayerMovePhase:
		OnStartResolvePlayerMovesRequested.ExecuteIfBound();
		break;
	case ETurnPhaseState::PlayerTurnPhase:
		OnTurnEndRequested.ExecuteIfBound();
		break;
	default:
		break;
	}
}

void ACardStage::ResetSelectedDeckBox()
{
	if (CurrentSelectedDeckBox.IsValid() && CurrentTurnPhaseState == ETurnPhaseState::PlayerMovePhase)
	{
		DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Pinned, false);
		CardContainerManager->StopPreviewDeck();
		CurrentSelectedDeckBox.Reset();
	}
}

ACardActor* ACardStage::GetCardActorAtUV(const FVector2D& TargetUV) const
{
	FHitResult OutHitResult;
	if (TryGetHitResultByCardChannel(TargetUV, OutHitResult))
	{
		return Cast<ACardActor>(OutHitResult.GetActor());
	}
	return nullptr;
}

UBoxComponent* ACardStage::GetDeckBoxCollisionAtUV(const FVector2D& TargetUV) const
{
	FHitResult OutHitResult;
	if (TryGetHitResultByCardChannel(TargetUV, OutHitResult))
	{
		return Cast<UBoxComponent>(OutHitResult.GetComponent());
	}
	return nullptr;
}

bool ACardStage::TryGetHitResultByCardChannel(const FVector2D& TargetUV, FHitResult& OutHitResult) const
{
	if (!CaptureComponent || TargetUV.X < 0.f || TargetUV.X > 1.f || TargetUV.Y < 0.f || TargetUV.Y > 1.f)
	{
		return false;
	}

	FVector TraceStart;
	FVector TraceDirection;
	if (!UGameplayStatics::DeprojectSceneCaptureComponentToWorld(CaptureComponent, TargetUV, TraceStart, TraceDirection))
	{
		return false;
	}

	const FVector TraceEnd = TraceStart + TraceDirection * CardTraceDistance;
	if (GetWorld()->LineTraceSingleByChannel(OutHitResult, TraceStart, TraceEnd, ECC_Card))
	{
		return true;
	}
	return false;
}

void ACardStage::OnCardMouseEvent(const ACardActor* CardActor, const ECardAction CardAction) const
{
	switch (CurrentTurnPhaseState)
	{
	case ETurnPhaseState::PlayerMovePhase:
	case ETurnPhaseState::PlayerTurnPhase:
		OnMouseEventWhenPlayerTurnPhase(CardActor, CardAction);
		break;
	default:
		break;
	}
}

void ACardStage::OnMouseEventWhenPlayerTurnPhase(const ACardActor* CardActor, const ECardAction CardAction) const
{
	switch (CardAction)
	{
	case ECardAction::Select:
		RequestSelectCard(CardActor);
		break;
	case ECardAction::ViewDetail:
		OnViewCardDetailRequested.ExecuteIfBound(CardActor);
		break;
	default:
		break;
	}
}

void ACardStage::OnKeyboardEventWhenDrawPhase(const int32 Number) const
{
	if (!CardContainerManager)
	{
		return;
	}

	if (AbilitySystemComponents.IsValidIndex(Number))
	{
		TryDraw(AbilitySystemComponents[Number].Get());
	}
}

void ACardStage::OnKeyboardEventWhenPlayerPhase(const int32 Number) const
{
	if (!CardContainerManager)
	{
		return;
	}

	const TArray<FHandSlot>& CurrentHandSlots = CardContainerManager->GetCurrentHandSlots();
	if (CurrentHandSlots.IsValidIndex(Number))
	{
		if (ACardActor* SelectingCard = CurrentHandSlots[Number].GetCard())
		{
			RequestSelectCard(SelectingCard);
		}
	}
}

void ACardStage::UpdateAllCardLocations() const
{
	if (CardContainerManager && DeckBoxes)
	{
		CardContainerManager->MoveAllCards();
	}
}

void ACardStage::TryDraw(ULetheAbilitySystemComponent* DeckOwnerASC) const
{
	if (!CardContainerManager || !DeckOwnerASC)
	{
		return;
	}

	if (CardContainerManager->AddCardToHandSlot(DeckOwnerASC))
	{
		UpdateAllCardLocations();

		if (DrawSoundTag.IsValid())
		{
			if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
			{
				FXManagerSubsystem->AsyncPlaySound2D(DrawSoundTag, 1.f, 1.f);
			}
		}

		// 8개의 핸드 슬롯을 모두 채웠다면 PlayerTurnPhase로 넘어갑니다.
		if (CardContainerManager->GetCurrentHandSlotCount() >= MAX_HAND_SLOT_COUNT)
		{
			OnDrawPhaseCompleted.ExecuteIfBound();
		}
	}
}

void ACardStage::RequestSelectCard(const ACardActor* CardActor) const
{
	if (!CardActor || !CardContainerManager)
	{
		return;
	}

	const int32 HandSlotIndex = CardContainerManager->FindCurrentHandSlotIndex(CardActor);
	if (UseRequestedCards.Contains(HandSlotIndex))
	{
		return;
	}

	if (CardActor->GetCurrentCardContainer() == ECardContainer::Graveyard || CardActor->GetCurrentCardContainer() == ECardContainer::Selected)
	{
		return;
	}

	OnSelectCardRequested.ExecuteIfBound(HandSlotIndex, CardActor->GetOwnerASC(), CardActor->GetAbilitySpecHandle());
}

void ACardStage::OnDrawPhaseStarted() const
{
	if (CardContainerManager && CardContainerManager->AreAllDecksEmpty())
	{
		CardContainerManager->StopPreviewDeck(false);
		CardContainerManager->RefillDeck();
		CardContainerManager->ShuffleDeck();
		UpdateAllCardLocations();
	}
}
