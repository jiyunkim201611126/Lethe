// Copyright JETBLU, Inc. All Rights Reserved.

#include "HandStage.h"

#include "CardActor.h"
#include "CardLayoutManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"

AHandStage::AHandStage()
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

void AHandStage::BeginPlay()
{
	Super::BeginPlay();

	CardLayoutManager = NewObject<UCardLayoutManager>(this);
	UseRequestedCards.Reserve(MAX_HAND_COUNT);
}

void AHandStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnViewCardDetailRequested.Unbind();
	OnSelectCardRequested.Unbind();
	OnGoPlayerTurnPhaseRequested.Unbind();
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

	CardLayoutManager = nullptr;

	Super::EndPlay(EndPlayReason);
}

void AHandStage::Initialize(const FVector2D& CardSize, const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents)
{
	AbilitySystemComponents = InAbilitySystemComponents;

	if (bInitialized || !CardLayoutManager)
	{
		return;
	}

	CardLayoutManager->Initialize(CardSize, GetActorTransform());
	bInitialized = true;

	if (CaptureComponent->TextureTarget)
	{
		CaptureComponent->TextureTarget->ClearColor = FLinearColor::Transparent;
	}
}

void AHandStage::CreateCard(const FCardInitParams& CardInitParams)
{
	if (!CardLayoutManager || !CardActorClass)
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

		CardLayoutManager->AddCardToDeck(CreatedCard);
		if (CardLayoutManager->AreAllDecksFull())
		{
			CardLayoutManager->ShuffleDeck();
			UpdateAllCardLocations();
		}

		CaptureComponent->ShowOnlyActors.Add(CreatedCard);
	}
}

void AHandStage::HandleCapturedMouseMove(const FVector2D& TargetUV)
{
	ACardActor* DetectedCard = GetCardActorAtUV(TargetUV);
	if (HoveredCard.Get() == DetectedCard)
	{
		return;
	}

	// HoveredCard가 기존과 다른 경우, 기존 카드는 MouseLeave 이벤트를 넘겨줍니다.
	if (HoveredCard.IsValid())
	{
		HoveredCard->HandleCardMouseEvent(ECardMouseEvent::MouseLeave);
	}

	// HoveredCard를 갱신하고 MouseEnter 이벤트를 넘겨줍니다.
	HoveredCard = DetectedCard;
	if (HoveredCard.IsValid())
	{
		HoveredCard->HandleCardMouseEvent(ECardMouseEvent::MouseEnter);
	}
}

bool AHandStage::HandleCapturedMouseButtonDown(const FVector2D& TargetUV, const FKey& MouseButton)
{
	if (MouseButton != EKeys::LeftMouseButton && MouseButton != EKeys::RightMouseButton)
	{
		return false;
	}

	// MouseMove 처리 함수를 명시적으로 호출, HoveredCard를 갱신합니다.
	HandleCapturedMouseMove(TargetUV);
	PressedCard = HoveredCard;

	// PressedCard가 유효하면 true를 반환합니다.
	return PressedCard.IsValid();
}

bool AHandStage::HandleCapturedMouseButtonUp(const FVector2D& TargetUV, const FKey& MouseButton)
{
	if (MouseButton != EKeys::LeftMouseButton && MouseButton != EKeys::RightMouseButton)
	{
		return false;
	}

	if (!PressedCard.IsValid())
	{
		return false;
	}

	// MouseMove 처리 함수를 명시적으로 호출, HoveredCard를 갱신합니다.
	HandleCapturedMouseMove(TargetUV);

	ACardActor* ReleasedCard = PressedCard.Get();
	PressedCard.Reset();

	// 마우스를 누른 카드와 뗀 카드가 다르면 클릭으로 처리하지 않습니다.
	if (ReleasedCard != HoveredCard.Get())
	{
		// 클릭으로 처리하진 않더라도 마우스 입력을 정상적으로 처리했으므로 true를 반환합니다.
		return true;
	}

	ReleasedCard->HandleCardMouseEvent(MouseButton == EKeys::LeftMouseButton ? ECardMouseEvent::LeftMouseButtonUp : ECardMouseEvent::RightMouseButtonUp);
	return true;
}

void AHandStage::HandleCapturedMouseLeave()
{
	if (HoveredCard.IsValid())
	{
		HoveredCard->HandleCardMouseEvent(ECardMouseEvent::MouseLeave);
		HoveredCard.Reset();
	}
}

void AHandStage::HandleCapturedMouseCaptureLost()
{
	if (PressedCard.IsValid())
	{
		PressedCard->HandleCardMouseEvent(ECardMouseEvent::MouseCaptureLost);
		PressedCard.Reset();
	}
}

bool AHandStage::HandleMouseButtonDownInCardUseSection() const
{
	return CurrentSelectedCard != nullptr;
}

bool AHandStage::HandleMouseButtonUpInCardUseSection()
{
	if (!CurrentSelectedCard || !CardLayoutManager || !OnUseCardRequested.IsBound() || !OnSelectCardRequested.IsBound())
	{
		return false;
	}

	const int32 HandIndex = CardLayoutManager->FindCurrentHandIndex(CurrentSelectedCard);
	if (HandIndex == INDEX_NONE)
	{
		CancelSelectedCard();
		return false;
	}

	// 사용 요청된 카드임을 기록하고 콜백 함수를 호출합니다.
	UseRequestedCards.Add(HandIndex, CurrentSelectedCard);
	OnUseCardRequested.Execute(CurrentSelectedCard->GetOwnerASC(), CurrentSelectedCard->GetSavedCard(), HandIndex);

	// 사용 요청이 완료되었으므로 카드 선택은 취소해둡니다.
	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->MouseHovered(false);
		CurrentSelectedCard = nullptr;
		OnSelectCardRequested.Execute(false, nullptr, FGameplayTag());
	}
	return true;
}

void AHandStage::HandleKeyboardEvent(const int32 Number)
{
	switch (CurrentPhaseState)
	{
	case EPhaseState::DrawPhase:
		OnKeyboardEventWhenDrawPhase(Number);
		break;
	case EPhaseState::PlayerTurnPhase:
		OnKeyboardEventWhenPlayerTurnPhase(Number);
		break;
	default:
		break;
	}
}

void AHandStage::HandlePhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState)
{
	CurrentPhaseState = NewState;

	if (CurrentPhaseState == EPhaseState::DrawPhase)
	{
		OnDrawPhaseStarted();
	}
}

void AHandStage::HandleCancelSelectedCard()
{
	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->SetCardContainer(ECardContainer::Hand, true);
		CurrentSelectedCard = nullptr;
		UpdateAllCardLocations();
	}
}

void AHandStage::HandleResolveUseCard(const int32 HandIndex, const bool bSuccess)
{
	ACardActor* CardActor = UseRequestedCards.FindRef(HandIndex);
	if (!CardActor)
	{
		return;
	}

	if (bSuccess)
	{
		if (CardLayoutManager)
		{
			CardLayoutManager->AddCardToGrave(CardActor);
		}
	}
	else
	{
		CardActor->SetCardContainer(ECardContainer::Hand, true);
	}

	UseRequestedCards.Remove(HandIndex);
}

void AHandStage::HandleTurnEndButtonClicked()
{
	switch (CurrentPhaseState)
	{
	case EPhaseState::PlayerMovePhase:
		OnStartResolvePlayerMovesRequested.ExecuteIfBound();
		break;
	case EPhaseState::PlayerTurnPhase:
		if (OnTurnEndRequested.IsBound() && OnTurnEndRequested.Execute())
		{
			if (CardLayoutManager)
			{
				CardLayoutManager->AddAllHandsToGrave();
			}
			UpdateAllCardLocations();
		}
		break;
	default:
		break;
	}
}

void AHandStage::CancelSelectedCard() const
{
	if (CurrentSelectedCard && OnSelectCardRequested.IsBound())
	{
		OnSelectCardRequested.Execute(false, nullptr, FGameplayTag());
	}
}

ACardActor* AHandStage::GetCardActorAtUV(const FVector2D& TargetUV) const
{
	if (!CaptureComponent || TargetUV.X < 0.f || TargetUV.X > 1.f || TargetUV.Y < 0.f || TargetUV.Y > 1.f)
	{
		return nullptr;
	}

	FVector TraceStart;
	FVector TraceDirection;
	if (!UGameplayStatics::DeprojectSceneCaptureComponentToWorld(CaptureComponent, TargetUV, TraceStart, TraceDirection))
	{
		return nullptr;
	}

	FHitResult HitResult;
	const FVector TraceEnd = TraceStart + TraceDirection * CardTraceDistance;
	if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Card))
	{
		return Cast<ACardActor>(HitResult.GetActor());
	}
	return nullptr;
}

void AHandStage::OnCardMouseEvent(ACardActor* CardActor, const ECardAction CardAction)
{
	switch (CurrentPhaseState)
	{
	case EPhaseState::DrawPhase:
		OnMouseEventWhenDrawPhase(CardActor, CardAction);
		break;
	case EPhaseState::PlayerTurnPhase:
		OnMouseEventWhenPlayerTurnPhase(CardActor, CardAction);
		break;
	default:
		break;
	}
}

void AHandStage::OnMouseEventWhenDrawPhase(const ACardActor* CardActor, const ECardAction CardAction)
{
	switch (CardAction)
	{
	case ECardAction::DeckHovered:
		OnDeckHovered(CardActor, true);
		break;
	case ECardAction::DeckUnhovered:
		OnDeckHovered(CardActor, false);
		break;
	case ECardAction::Draw:
		TryDraw(CardActor->GetOwnerASC());
		break;
	default:
		break;
	}
}

void AHandStage::OnMouseEventWhenPlayerTurnPhase(ACardActor* CardActor, const ECardAction CardAction)
{
	switch (CardAction)
	{
	case ECardAction::HandHovered:
		OnHandHovered(CardActor, true);
		break;
	case ECardAction::HandUnhovered:
		OnHandHovered(CardActor, false);
		break;
	case ECardAction::Selected:
		SelectCard(CardActor);
		break;
	case ECardAction::ViewDetail:
		OnViewCardDetailRequested.ExecuteIfBound(CardActor);
		break;
	default:
		break;
	}
}

void AHandStage::OnKeyboardEventWhenDrawPhase(const int32 Number)
{
	if (!CardLayoutManager)
	{
		return;
	}

	if (AbilitySystemComponents.IsValidIndex(Number))
	{
		TryDraw(AbilitySystemComponents[Number]);
	}
}

void AHandStage::OnKeyboardEventWhenPlayerTurnPhase(const int32 Number)
{
	if (!CardLayoutManager)
	{
		return;
	}

	const TArray<TObjectPtr<ACardActor>>& CurrentHands = CardLayoutManager->GetCurrentHands();
	if (CurrentHands.IsValidIndex(Number))
	{
		ACardActor* SelectedCard = CurrentHands[Number];
		if (SelectedCard && SelectedCard->GetCurrentCardContainer() == ECardContainer::Hand)
		{
			SelectCard(SelectedCard);
		}
	}
}

void AHandStage::UpdateAllCardLocations() const
{
	if (CardLayoutManager)
	{
		CardLayoutManager->MoveAllCards(AbilitySystemComponents);
	}
}

void AHandStage::OnDeckHovered(const ACardActor* CardActor, const bool bHovered) const
{
	if (!CardLayoutManager || !CardActor)
	{
		return;
	}

	if (ULetheAbilitySystemComponent* OwnerASC = CardActor->GetOwnerASC())
	{
		if (ACardActor* DeckOnTopCard = CardLayoutManager->GetTopCardFromDeck(OwnerASC))
		{
			DeckOnTopCard->MouseHovered(bHovered);
		}
	}
}

void AHandStage::TryDraw(ULetheAbilitySystemComponent* OwnerASC) const
{
	if (!CardLayoutManager || !OwnerASC)
	{
		return;
	}

	if (CardLayoutManager->TryDraw(OwnerASC))
	{
		UpdateAllCardLocations();
	}

	// 8장을 모두 드로우했다면 PlayerTurnPhase로 넘어갑니다.
	if (CardLayoutManager->GetCurrentHandsNum() >= MAX_HAND_COUNT)
	{
		OnGoPlayerTurnPhaseRequested.ExecuteIfBound();

		for (ULetheAbilitySystemComponent* AbilitySystemComponent : AbilitySystemComponents)
		{
			if (ACardActor* DeckOnTopCard = CardLayoutManager->GetTopCardFromDeck(AbilitySystemComponent))
			{
				DeckOnTopCard->MouseHovered(false);
			}
		}
	}
}

void AHandStage::OnHandHovered(ACardActor* CardActor, const bool bHovered) const
{
	if (CardActor)
	{
		CardActor->MouseHovered(bHovered);
	}
}

void AHandStage::SelectCard(ACardActor* CardActor)
{
	if (!CardActor || !CardLayoutManager)
	{
		return;
	}

	CancelSelectedCard();

	const int32 HandIndex = CardLayoutManager->FindCurrentHandIndex(CardActor);
	if (UseRequestedCards.Contains(HandIndex))
	{
		return;
	}

	if (CardActor->GetCurrentCardContainer() == ECardContainer::Grave || CardActor->GetCurrentCardContainer() == ECardContainer::Selected)
	{
		return;
	}

	CurrentSelectedCard = CardActor;
	if (OnSelectCardRequested.IsBound() && CurrentSelectedCard)
	{
		if (OnSelectCardRequested.Execute(true, CurrentSelectedCard->GetOwnerASC(), CurrentSelectedCard->GetCardTag()))
		{
			CurrentSelectedCard->SetCardContainer(ECardContainer::Selected);
		}
	}
}

void AHandStage::OnDrawPhaseStarted() const
{
	if (OnSelectCardRequested.IsBound())
	{
		OnSelectCardRequested.Execute(false, nullptr, FGameplayTag());
	}

	if (CardLayoutManager && CardLayoutManager->AreAllDecksEmpty())
	{
		CardLayoutManager->RefillDeck();
		CardLayoutManager->ShuffleDeck();
		UpdateAllCardLocations();
	}
}
