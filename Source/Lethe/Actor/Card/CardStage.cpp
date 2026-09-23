// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardStage.h"

#include "CardActor.h"
#include "DeckBoxes.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Components/BoxComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/Lethe.h"
#include "Lethe/LetheLog.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Character/LetheCharacterBase.h"
#include "Lethe/Game/GameState/CardContainerComponent.h"
#include "Lethe/Game/GameState/LetheGameState.h"
#include "Lethe/Manager/FX/FXManagerSubsystem.h"
#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"

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
		CardContainer = LetheGameState->GetCardContainerComponent();
	}

	if (CardContainer.IsValid())
	{
		CardContainer->OnContainerChanged.AddUObject(this, &ThisClass::OnContainerChanged);
		bContainerDelegateBound = true;
	}

	DeckBoxes = GetWorld()->SpawnActor<ADeckBoxes>(DeckBoxesClass);
	DeckBoxes->SetActorTransform(GetActorTransform());

	UseRequestedCards.Reserve(MAX_HAND_SLOT_COUNT);
	CurrentSlots.Reserve(MAX_HAND_SLOT_COUNT);

	CaptureComponent->ShowOnlyActors.Add(DeckBoxes);
}

void ACardStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		LetheGameState->OnChangeTurnPhaseState.RemoveAll(this);
	}

	if (bContainerDelegateBound && CardContainer.IsValid())
	{
		CardContainer->OnContainerChanged.RemoveAll(this);
	}
	bContainerDelegateBound = false;

	OnViewCardDetailRequested.Unbind();
	OnSelectCardRequested.Unbind();
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
	CardActorsByInstance.Reset();

	DeckBoxes = nullptr;

	Super::EndPlay(EndPlayReason);
}

void ACardStage::Initialize(const TArray<TObjectPtr<ULetheAbilitySystemComponent>>& InAbilitySystemComponents)
{
	if (!DeckBoxes)
	{
		return;
	}

	// 모든 캐릭터가 넘어올 때까지 계속해서 ASC를 캐싱합니다.
	AbilitySystemComponents.Reset();
	AbilitySystemComponents.Reserve(InAbilitySystemComponents.Num());
	for (ULetheAbilitySystemComponent* ASC : InAbilitySystemComponents)
	{
		AbilitySystemComponents.Add(ASC);
	}

	// 새 캐릭터가 등록되면 배치 기준(덱 박스 인덱스)이 바뀌므로, 카드 액터가 준비된 경우 다시 배치합니다.
	if (AreAllCardActorsBound())
	{
		RefreshCardContainerStates();
		UpdateAllCardLocations();
	}

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
	if (!CardActorClass || !CardContainer.IsValid())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ACardActor* CreatedCard = GetWorld()->SpawnActor<ACardActor>(CardActorClass, GetActorTransform(), SpawnParams);
	if (!CreatedCard)
	{
		return;
	}

	CreatedCard->AttachToActor(this, FAttachmentTransformRules::KeepWorldTransform);
	CreatedCard->SetCardInfo(CardInitParams);
	CreatedCard->OnCardMouseEventDelegate.BindUObject(this, &ThisClass::OnCardMouseEvent);
	SpawnedCards.Add(CreatedCard);
	CaptureComponent->ShowOnlyActors.Add(CreatedCard);

	// 모델에서 같은 캐릭터/카드/레벨이면서 아직 액터가 연결되지 않은 인스턴스를 찾아 연결합니다.
	// Ability 부여 순서에 의존하지 않으므로, 로드 후 Ability를 다시 부여하는 경우에도 같은 규칙으로 매칭됩니다.
	const FGameplayTag OwnerCharacterTag = GetCharacterTag(CardInitParams.CardOwnerASC);
	const FSavedCard& SavedCard = CardInitParams.SavedCard;
	for (const FCardInstance& Instance : CardContainer->GetInstances())
	{
		if (Instance.CharacterTag != OwnerCharacterTag || Instance.CardId != SavedCard.CardId || Instance.CardLevel != SavedCard.CardLevel)
		{
			continue;
		}
		if (CardActorsByInstance.Contains(Instance.InstanceId))
		{
			continue;
		}

		CreatedCard->SetInstanceId(Instance.InstanceId);
		CardActorsByInstance.Add(Instance.InstanceId, CreatedCard);
		break;
	}

	if (CreatedCard->GetInstanceId() == INDEX_NONE)
	{
		// 모델에 없는 카드는 게임 규칙상 존재하지 않는 카드이므로, 스테이지 원점에 남지 않도록 제거합니다.
		LETHE_LOG(LogLetheGameState, Warning, "카드 컨테이너에서 대응하는 인스턴스를 찾지 못해 카드 액터를 제거합니다. CardId: %llu, Character: %s", SavedCard.CardId, *OwnerCharacterTag.ToString());
		CaptureComponent->ShowOnlyActors.Remove(CreatedCard);
		SpawnedCards.Remove(CreatedCard);
		CreatedCard->OnCardMouseEventDelegate.Unbind();
		CreatedCard->Destroy();
		return;
	}

	// 모든 카드 액터가 모델과 연결되면 첫 배치를 수행합니다.
	if (AreAllCardActorsBound())
	{
		RefreshCardContainerStates();
		UpdateAllCardLocations();
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
			StopPreviewDeck(false);
			DeckBoxes->SetOpenReason(ClickedDeckBox, EDeckBoxOpenReason::Pinned, true);
			PreviewDeck(GetCharacterTag(ClickedDeckOwnerASC));
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
	if (!CurrentSelectedCard || !OnUseCardRequested.IsBound() || !OnSelectCardRequested.IsBound())
	{
		return false;
	}

	const int32 HandSlotIndex = FindCurrentSlotIndex(CurrentSelectedCard);
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
	// Deck/Hand/Graveyard 상태 변경(핸드 버림, 리필, 셔플)은 모델이 이 콜백보다 먼저 처리합니다. 여기서는 연출과 뷰 상태만 다룹니다.
	if (OldTurnPhaseState == ETurnPhaseState::PlayerTurnPhase)
	{
		if (TurnEndSoundTag.IsValid())
		{
			if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
			{
				FXManagerSubsystem->AsyncPlaySound2D(TurnEndSoundTag, 1.f, 1.f);
			}
		}
	}

	CurrentTurnPhaseState = NewTurnPhaseState;

	if (const ALetheGameState* LetheGameState = GetWorld()->GetGameState<ALetheGameState>())
	{
		// 페이즈가 바뀌면 덱 미리보기는 항상 종료합니다.
		StopPreviewDeck(false);
		CurrentSelectedDeckBox.Reset();

		if (LetheGameState->IsBattlePhase())
		{
			// 전투 중인 상황이므로 모든 덱 박스를 엽니다.
			DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Pinned, false);
			DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Battle, true);
		}
		else
		{
			DeckBoxes->SetAllOpenReason(EDeckBoxOpenReason::Battle, false);
		}
	}

	RefreshCardContainerStates();
	UpdateAllCardLocations();
}

void ACardStage::OnContainerChanged()
{
	if (!AreAllCardActorsBound())
	{
		// 아직 카드 액터가 모두 생성되지 않았다면, 생성이 끝나는 시점에 배치합니다.
		return;
	}

	RefreshCardContainerStates();
	UpdateAllCardLocations();
}

void ACardStage::RefreshCardContainerStates()
{
	if (!CardContainer.IsValid())
	{
		return;
	}

	// 모든 카드 액터의 표시 상태를 모델의 Zone으로부터 파생시킵니다. 선택 상태는 뷰 상태이므로 유지합니다.
	for (const auto& Pair : CardActorsByInstance)
	{
		ACardActor* CardActor = Pair.Value;
		if (!CardActor || CardActor == CurrentSelectedCard)
		{
			continue;
		}

		switch (CardContainer->GetZone(Pair.Key))
		{
		case ECardZone::Deck:
			CardActor->SetCardContainer(ECardContainer::Deck);
			break;
		case ECardZone::Hand:
			CardActor->SetCardContainer(ECardContainer::Hands);
			break;
		case ECardZone::Graveyard:
			CardActor->SetCardContainer(ECardContainer::Graveyard);
			break;
		default:
			break;
		}
	}
}

void ACardStage::UpdateAllCardLocations()
{
	if (!CardContainer.IsValid() || !DeckBoxes)
	{
		return;
	}

	// 드로우는 캐릭터 순서에 관계 없이 진행되므로, 캐릭터 순서에 맞춰 정렬된 핸드 슬롯을 얻기 위해 여기서 캐싱 로직을 수행합니다.
	CurrentSlots.Reset();

	// 덱 박스 위치 갱신을 위해 캐릭터별 핸드 줄의 카드 개수를 먼저 계산합니다.
	TArray<int32> HandSlotCounts;
	HandSlotCounts.Reserve(AbilitySystemComponents.Num());
	for (const auto& AbilitySystemComponent : AbilitySystemComponents)
	{
		const FGameplayTag CharacterTag = GetCharacterTag(AbilitySystemComponent.Get());
		const FCharacterCardZones* Zones = CardContainer->GetZones(CharacterTag);
		const bool bIsPreviewing = CharacterTag == PreviewCharacterTag;
		HandSlotCounts.Add(bIsPreviewing ? DeckPreviewInstanceIds.Num() : (Zones ? Zones->HandSlots.Num() : 0));
	}
	DeckBoxes->UpdateLocations(HandSlotCounts);

	TArray<FVector> DeckLocations;
	DeckBoxes->GetDeckLocations(DeckLocations);

	const auto PlaceCard = [this](const int32 InstanceId, const FVector& Location, const FRotator& Rotation)
	{
		if (ACardActor* CardActor = FindCardActor(InstanceId))
		{
			CardActor->SetActorLocation(Location);
			CardActor->SetActorRotation(Rotation);
		}
	};

	for (int32 Index = 0; Index < AbilitySystemComponents.Num(); ++Index)
	{
		if (!DeckLocations.IsValidIndex(Index))
		{
			return;
		}

		const FGameplayTag CharacterTag = GetCharacterTag(AbilitySystemComponents[Index].Get());
		const FCharacterCardZones* Zones = CardContainer->GetZones(CharacterTag);
		if (!Zones)
		{
			continue;
		}

		const FVector DeckLocation = DeckLocations[Index];

		FVector DeckCardLocation = DeckLocation + DeckFirstCardLocation;
		for (const int32 InstanceId : Zones->Deck)
		{
			DeckCardLocation += FVector(DeckCardXOffset, 0.f, 0.f);
			PlaceCard(InstanceId, DeckCardLocation, FRotator(0.f, -90.f, 0.f));
		}

		FVector HandSlotCardLocation = DeckLocation + HandSlotFirstCardLocation;
		if (CharacterTag == PreviewCharacterTag)
		{
			// 덱 미리보기 중인 캐릭터는 핸드 줄에 정렬된 덱 카드를 펼쳐 보여줍니다.
			for (const int32 InstanceId : DeckPreviewInstanceIds)
			{
				CurrentSlots.Add(InstanceId);
				HandSlotCardLocation += FVector(HandSlotCardXOffset, 0.f, 0.f);
				PlaceCard(InstanceId, HandSlotCardLocation, FRotator::ZeroRotator);
			}
		}
		else
		{
			for (const int32 InstanceId : Zones->HandSlots)
			{
				// 카드를 사용해도 슬롯은 유지되므로, 비어 있는 슬롯(INDEX_NONE)도 CurrentSlots에 포함합니다.
				CurrentSlots.Add(InstanceId);
				HandSlotCardLocation += FVector(HandSlotCardXOffset, 0.f, 0.f);
				PlaceCard(InstanceId, HandSlotCardLocation, FRotator::ZeroRotator);
			}
		}

		FVector GraveyardCardLocation = DeckLocation + GraveyardFirstCardLocation;
		for (const int32 InstanceId : Zones->Graveyard)
		{
			GraveyardCardLocation += GraveyardCardOffset;
			PlaceCard(InstanceId, GraveyardCardLocation, FRotator(0.f, 180.f, -60.f));
		}
	}
}

void ACardStage::PreviewDeck(const FGameplayTag& CharacterTag)
{
	const FCharacterCardZones* Zones = CardContainer.IsValid() ? CardContainer->GetZones(CharacterTag) : nullptr;
	if (!Zones)
	{
		return;
	}

	PreviewCharacterTag = CharacterTag;
	DeckPreviewInstanceIds = Zones->Deck;

	// 카드 Id 순서대로(물리, 마법, 보조 순서) 보여주기 위해 덱 미리보기 카드를 한 번 정렬합니다.
	Algo::Sort(DeckPreviewInstanceIds, [this](const int32 A, const int32 B)
	{
		const FCardInstance* InstanceA = CardContainer->FindInstance(A);
		const FCardInstance* InstanceB = CardContainer->FindInstance(B);
		return InstanceA && InstanceB && InstanceA->CardId < InstanceB->CardId;
	});

	UpdateAllCardLocations();
}

void ACardStage::StopPreviewDeck(const bool bShouldMoveCards)
{
	PreviewCharacterTag = FGameplayTag::EmptyTag;
	DeckPreviewInstanceIds.Reset();

	if (bShouldMoveCards)
	{
		UpdateAllCardLocations();
	}
}

void ACardStage::OnCardSelected(const int32 HandSlotIndex)
{
	if (CurrentSlots.IsValidIndex(HandSlotIndex))
	{
		CurrentSelectedCard = FindCardActor(CurrentSlots[HandSlotIndex]);
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
		CurrentSelectedCard = nullptr;
		RefreshCardContainerStates();
	}
}

void ACardStage::OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess)
{
	ACardActor* CardActor = UseRequestedCards.FindRef(HandSlotIndex);
	if (!CardActor)
	{
		return;
	}
	UseRequestedCards.Remove(HandSlotIndex);

	if (CurrentTurnPhaseState == ETurnPhaseState::PlayerTurnPhase && bSuccess && CardContainer.IsValid())
	{
		// 모델이 변경되면 OnContainerChanged를 통해 표시 상태와 위치가 갱신됩니다.
		CardContainer->MoveToGraveyard(CardActor->GetInstanceId());
		return;
	}

	// 비전투 중 덱 미리보기에서 사용한 카드는 모델상 덱을 떠난 적이 없으므로, 뷰 상태만 되돌립니다. 사용 실패 시에도 마찬가지입니다.
	RefreshCardContainerStates();
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
		StopPreviewDeck();
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
	if (AbilitySystemComponents.IsValidIndex(Number))
	{
		TryDraw(AbilitySystemComponents[Number].Get());
	}
}

void ACardStage::OnKeyboardEventWhenPlayerPhase(const int32 Number) const
{
	if (CurrentSlots.IsValidIndex(Number))
	{
		if (const ACardActor* SelectingCard = FindCardActor(CurrentSlots[Number]))
		{
			RequestSelectCard(SelectingCard);
		}
	}
}

void ACardStage::TryDraw(ULetheAbilitySystemComponent* DeckOwnerASC) const
{
	if (!CardContainer.IsValid() || !DeckOwnerASC)
	{
		return;
	}

	// 드로우 결과 배치는 모델의 OnContainerChanged로, 핸드가 모두 채워졌을 때의 페이즈 전환은 모델이 GameState에 직접 알립니다.
	int32 DrawnInstanceId = INDEX_NONE;
	if (CardContainer->DrawTopCard(GetCharacterTag(DeckOwnerASC), DrawnInstanceId))
	{
		if (DrawSoundTag.IsValid())
		{
			if (UFXManagerSubsystem* FXManagerSubsystem = GetWorld()->GetGameInstance()->GetSubsystem<UFXManagerSubsystem>())
			{
				FXManagerSubsystem->AsyncPlaySound2D(DrawSoundTag, 1.f, 1.f);
			}
		}
	}
}

void ACardStage::RequestSelectCard(const ACardActor* CardActor) const
{
	if (!CardActor)
	{
		return;
	}

	const int32 HandSlotIndex = FindCurrentSlotIndex(CardActor);
	if (HandSlotIndex == INDEX_NONE || UseRequestedCards.Contains(HandSlotIndex))
	{
		return;
	}

	if (CardActor->GetCurrentCardContainer() == ECardContainer::Graveyard || CardActor->GetCurrentCardContainer() == ECardContainer::Selected)
	{
		return;
	}

	OnSelectCardRequested.ExecuteIfBound(HandSlotIndex, CardActor->GetOwnerASC(), CardActor->GetAbilitySpecHandle());
}

ACardActor* ACardStage::FindCardActor(const int32 InstanceId) const
{
	if (InstanceId == INDEX_NONE)
	{
		return nullptr;
	}
	return CardActorsByInstance.FindRef(InstanceId);
}

int32 ACardStage::FindCurrentSlotIndex(const ACardActor* CardActor) const
{
	if (!CardActor || CardActor->GetInstanceId() == INDEX_NONE)
	{
		return INDEX_NONE;
	}
	return CurrentSlots.IndexOfByKey(CardActor->GetInstanceId());
}

FGameplayTag ACardStage::GetCharacterTag(const ULetheAbilitySystemComponent* AbilitySystemComponent) const
{
	if (AbilitySystemComponent)
	{
		if (ALetheCharacterBase* OwnerCharacter = Cast<ALetheCharacterBase>(AbilitySystemComponent->GetAvatarActor()))
		{
			return OwnerCharacter->GetCharacterTag();
		}
	}
	return FGameplayTag::EmptyTag;
}

bool ACardStage::AreAllCardActorsBound() const
{
	if (!CardContainer.IsValid() || !CardContainer->IsInitialized() || AbilitySystemComponents.IsEmpty())
	{
		return false;
	}

	// 현재 스테이지에 등록된 캐릭터들의 인스턴스만 셉니다. 아직 등록되지 않은 캐릭터의 카드는 기다리지 않습니다.
	TSet<FGameplayTag> RegisteredCharacterTags;
	for (const auto& AbilitySystemComponent : AbilitySystemComponents)
	{
		RegisteredCharacterTags.Add(GetCharacterTag(AbilitySystemComponent.Get()));
	}

	int32 ExpectedCount = 0;
	for (const FCardInstance& Instance : CardContainer->GetInstances())
	{
		if (RegisteredCharacterTags.Contains(Instance.CharacterTag))
		{
			++ExpectedCount;
		}
	}
	return ExpectedCount > 0 && CardActorsByInstance.Num() >= ExpectedCount;
}
