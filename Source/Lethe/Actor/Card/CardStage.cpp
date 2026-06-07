// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardStage.h"

#include "CardActor.h"
#include "CardContainerManager.h"
#include "DeckBoxes.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"

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

	CardContainerManager = NewObject<UCardContainerManager>(this);
	UseRequestedCards.Reserve(MAX_HAND_COUNT);

	DeckBoxes = GetWorld()->SpawnActor<ADeckBoxes>(DeckBoxesClass);
	DeckBoxes->SetActorTransform(GetActorTransform());

	CaptureComponent->ShowOnlyActors.Add(DeckBoxes);
}

void ACardStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
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

bool ACardStage::HandleCapturedMouseButtonDown(const FVector2D& TargetUV, const FKey& MouseButton)
{
	if (MouseButton != EKeys::LeftMouseButton && MouseButton != EKeys::RightMouseButton)
	{
		return false;
	}

	// MouseMove 처리 함수를 명시적으로 호출, HoveredCard를 갱신합니다.
	PressedCard = GetCardActorAtUV(TargetUV);

	// PressedCard가 유효하면 true를 반환합니다.
	return PressedCard.IsValid();
}

bool ACardStage::HandleCapturedMouseButtonUp(const FVector2D& TargetUV, const FKey& MouseButton)
{
	if (MouseButton != EKeys::LeftMouseButton && MouseButton != EKeys::RightMouseButton)
	{
		return false;
	}

	if (!PressedCard.IsValid())
	{
		return false;
	}

	const ACardActor* HoveredCard = GetCardActorAtUV(TargetUV);
	ACardActor* ReleasedCard = PressedCard.Get();
	PressedCard.Reset();

	// 마우스를 누른 카드와 뗀 카드가 다르면 클릭으로 처리하지 않습니다.
	if (ReleasedCard != HoveredCard)
	{
		// 클릭으로 처리하진 않더라도 마우스 입력을 정상적으로 처리했으므로 true를 반환합니다.
		return true;
	}

	ReleasedCard->HandleCardMouseEvent(MouseButton == EKeys::LeftMouseButton ? ECardMouseEvent::LeftMouseButtonUp : ECardMouseEvent::RightMouseButtonUp);
	return true;
}

void ACardStage::HandleCapturedMouseCaptureLost()
{
	if (PressedCard.IsValid())
	{
		PressedCard->HandleCardMouseEvent(ECardMouseEvent::MouseCaptureLost);
		PressedCard.Reset();
	}
}

bool ACardStage::HandleMouseButtonDownInCardUseSection() const
{
	return CurrentSelectedCard != nullptr;
}

bool ACardStage::HandleMouseButtonUpInCardUseSection()
{
	if (!CurrentSelectedCard || !CardContainerManager || !OnUseCardRequested.IsBound() || !OnSelectCardRequested.IsBound())
	{
		return false;
	}

	const int32 HandIndex = CardContainerManager->FindCurrentHandIndex(CurrentSelectedCard);
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
		CurrentSelectedCard = nullptr;
		OnSelectCardRequested.Execute(false, nullptr, FGameplayTag());
	}
	return true;
}

void ACardStage::HandleKeyboardEvent(const int32 Number)
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

void ACardStage::HandlePhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState)
{
	CurrentPhaseState = NewState;

	if (CurrentPhaseState == EPhaseState::DrawPhase)
	{
		OnDrawPhaseStarted();
	}
}

void ACardStage::HandleCancelSelectedCard()
{
	if (CurrentSelectedCard)
	{
		CurrentSelectedCard->SetCardContainer(ECardContainer::Hand, true);
		CurrentSelectedCard = nullptr;
	}
}

void ACardStage::HandleResolveUseCard(const int32 HandIndex, const bool bSuccess)
{
	ACardActor* CardActor = UseRequestedCards.FindRef(HandIndex);
	if (!CardActor)
	{
		return;
	}

	if (bSuccess)
	{
		if (CardContainerManager)
		{
			CardContainerManager->AddCardToGrave(CardActor);
		}
		UpdateAllCardLocations();
	}
	else
	{
		CardActor->SetCardContainer(ECardContainer::Hand, true);
	}

	UseRequestedCards.Remove(HandIndex);
}

void ACardStage::HandleTurnEndButtonClicked()
{
	switch (CurrentPhaseState)
	{
	case EPhaseState::PlayerMovePhase:
		OnStartResolvePlayerMovesRequested.ExecuteIfBound();
		break;
	case EPhaseState::PlayerTurnPhase:
		if (OnTurnEndRequested.IsBound() && OnTurnEndRequested.Execute())
		{
			if (CardContainerManager)
			{
				CardContainerManager->AddAllHandsToGrave();
			}
			UpdateAllCardLocations();
		}
		break;
	default:
		break;
	}
}

void ACardStage::CancelSelectedCard() const
{
	if (CurrentSelectedCard && OnSelectCardRequested.IsBound())
	{
		OnSelectCardRequested.Execute(false, nullptr, FGameplayTag());
	}
}

ACardActor* ACardStage::GetCardActorAtUV(const FVector2D& TargetUV) const
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

void ACardStage::OnCardMouseEvent(ACardActor* CardActor, const ECardAction CardAction)
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

void ACardStage::OnMouseEventWhenDrawPhase(const ACardActor* CardActor, const ECardAction CardAction) const
{
	switch (CardAction)
	{
	case ECardAction::Draw:
		TryDraw(CardActor->GetOwnerASC());
		break;
	default:
		break;
	}
}

void ACardStage::OnMouseEventWhenPlayerTurnPhase(ACardActor* CardActor, const ECardAction CardAction)
{
	switch (CardAction)
	{
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

void ACardStage::OnKeyboardEventWhenDrawPhase(const int32 Number)
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

void ACardStage::OnKeyboardEventWhenPlayerTurnPhase(const int32 Number)
{
	if (!CardContainerManager)
	{
		return;
	}

	const TArray<TObjectPtr<ACardActor>>& CurrentHands = CardContainerManager->GetCurrentHands();
	if (CurrentHands.IsValidIndex(Number))
	{
		ACardActor* SelectingCard = CurrentHands[Number];
		if (SelectingCard && SelectingCard->GetCurrentCardContainer() == ECardContainer::Hand)
		{
			SelectCard(SelectingCard);
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

void ACardStage::TryDraw(ULetheAbilitySystemComponent* OwnerASC) const
{
	if (!CardContainerManager || !OwnerASC)
	{
		return;
	}

	if (CardContainerManager->TryDraw(OwnerASC))
	{
		UpdateAllCardLocations();
	}

	// 8장을 모두 드로우했다면 PlayerTurnPhase로 넘어갑니다.
	if (CardContainerManager->GetCurrentHandCount() >= MAX_HAND_COUNT)
	{
		OnGoPlayerTurnPhaseRequested.ExecuteIfBound();
	}
}

void ACardStage::SelectCard(ACardActor* CardActor)
{
	if (!CardActor || !CardContainerManager)
	{
		return;
	}

	CancelSelectedCard();

	const int32 HandIndex = CardContainerManager->FindCurrentHandIndex(CardActor);
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
		else
		{
			CancelSelectedCard();
		}
	}
}

void ACardStage::OnDrawPhaseStarted() const
{
	if (OnSelectCardRequested.IsBound())
	{
		OnSelectCardRequested.Execute(false, nullptr, FGameplayTag());
	}

	if (CardContainerManager && CardContainerManager->AreAllDecksEmpty())
	{
		CardContainerManager->RefillDeck();
		CardContainerManager->ShuffleDeck();
		UpdateAllCardLocations();
	}
}
