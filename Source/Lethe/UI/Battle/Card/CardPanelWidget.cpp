// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardPanelWidgetController.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Lethe/Actor/Card/CardStage.h"
#include "Lethe/UI/Core/LetheImage.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	TurnEndButton->OnClicked.AddDynamic(this, &ThisClass::OnTurnEndButtonClicked);
}

void UCardPanelWidget::NativeDestruct()
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->OnAbilityUpdatedDelegate.Unbind();
		CardPanelWidgetController->OnAbilitySystemReferencesUpdatedDelegate.Unbind();
		CardPanelWidgetController->OnCardSelectedDelegate.Unbind();
		CardPanelWidgetController->OnCardSelectCanceledDelegate.Unbind();
		CardPanelWidgetController->OnUseCardResolvedDelegate.Unbind();
	}

	if (TurnEndButton)
	{
		TurnEndButton->OnClicked.RemoveDynamic(this, &ThisClass::OnTurnEndButtonClicked);
	}
	if (CardStage)
	{
		CardStage->Destroy();
		CardStage = nullptr;
	}

	DownPositions.Empty();

	Super::NativeDestruct();
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController);

	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
		CardPanelWidgetController->OnAbilitySystemReferencesUpdatedDelegate.BindUObject(this, &ThisClass::TryInitializeCardStage);
		CardPanelWidgetController->OnCardSelectedDelegate.BindUObject(this, &ThisClass::OnCardSelected);
		CardPanelWidgetController->OnCardSelectCanceledDelegate.BindUObject(this, &ThisClass::OnCancelSelectedCard);
		CardPanelWidgetController->OnUseCardResolvedDelegate.BindUObject(this, &ThisClass::OnResolveUseCard);

		if (CardStageClass && !CardStage)
		{
			const FVector StageLocation = FVector(0.f, 0.f, -3000.f);
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			CardStage = GetWorld()->SpawnActor<ACardStage>(CardStageClass, StageLocation, FRotator::ZeroRotator, SpawnParams);
			if (CardStage)
			{
				CardStage->OnViewCardDetailRequested.BindUObject(this, &ThisClass::StartViewCardDetail);
				CardStage->OnSelectCardRequested.BindUObject(this, &ThisClass::OnSelectCardRequested);
				CardStage->OnDrawPhaseCompleted.BindUObject(this, &ThisClass::NotifyDrawPhaseCompleted);
				CardStage->OnStartResolvePlayerMovesRequested.BindUObject(this, &ThisClass::StartResolvePlayerMoves);
				CardStage->OnUseCardRequested.BindUObject(this, &ThisClass::RequestUseCard);
				CardStage->OnTurnEndRequested.BindUObject(this, &ThisClass::RequestTurnEnd);
			}
		}

		TryInitializeCardStage();
	}
}

void UCardPanelWidget::TryInitializeCardStage() const
{
	if (CardStage && CardPanelWidgetController)
	{
		TArray<TObjectPtr<ULetheAbilitySystemComponent>> AbilitySystemComponents;
		for (const FAbilitySystemReference& AbilitySystemReference : CardPanelWidgetController->GetAbilitySystemReferences())
		{
			AbilitySystemComponents.Add(AbilitySystemReference.AbilitySystemComponent);
		}

		CardStage->Initialize(AbilitySystemComponents);
	}
}

FReply UCardPanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!CanRouteMouseInput())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton && InMouseEvent.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	FVector2D& DownPosition = DownPositions.FindOrAdd(InMouseEvent.GetEffectingButton());
	DownPosition = InMouseEvent.GetScreenSpacePosition();

	// 마우스 클릭은 모두 위젯을 거쳐 처리되므로 일단 캡쳐합니다.
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCardPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!CanRouteMouseInput())
	{
		DownPositions.Remove(InMouseEvent.GetEffectingButton());
		return MakeMouseUpReply();
	}
	
	FVector2D DownPosition;
	const bool bIsValid = DownPositions.RemoveAndCopyValue(InMouseEvent.GetEffectingButton(), DownPosition);
	if (!bIsValid || !CardStage || !CardPanelWidgetController)
	{
		return MakeMouseUpReply();
	}

	// Down 입력 후 Up 시점에 마우스가 기준 거리보다 더 많이 이동했다면 처리하지 않습니다.
	const FVector2D UpPosition = InMouseEvent.GetScreenSpacePosition();
	if (FVector2D::DistSquared(DownPosition, UpPosition) >= MaxClickDistanceSqr)
	{
		return MakeMouseUpReply();
	}
	
	FVector2D TargetUV;
	const bool bIsMouseInCardStage = TryGetCapturedCardStageUV(InMouseEvent, TargetUV);
	const bool bIsMouseInWorldSection = IsMouseInWorldSection(InMouseEvent);
	
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsMouseInCardStage)
		{
			if (CardStage->HandleLeftMouseButtonClickedInCardStageSection(TargetUV))
			{
				// 카드 선택, 드로우, 덱 프리뷰 등의 동작이 성공적으로 이루어진 경우 함수를 중단합니다.
				return MakeMouseUpReply();
			}
		}
		if (bIsMouseInWorldSection)
		{
			// 카드 선택도 사용도 하지 않았으며 마우스는 월드 섹션에 있는 경우, WorldSection 내 마우스 입력 처리를 시작합니다.
			const bool bHandled = CardStage->HandleLeftMouseButtonClickedInWorldSection();
			if (!bHandled)
			{
				CardPanelWidgetController->HandleLeftMouseButtonClickedInWorldSection();
			}
		}
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		// 먼저 ViewDetail을 시도합니다.
		bool bHandled = false;
		if (bIsMouseInCardStage)
		{
			bHandled = CardStage->TryViewDetail(TargetUV);
		}

		// ViewDetail이 동작하지 않았다면 카드 선택 취소를 시도합니다.
		if (!bHandled)
		{
			if (CardPanelWidgetController->IsCardSelected())
			{
				CardPanelWidgetController->ResetSelectedCard();
				bHandled = true;
			}
		}

		// 카드 선택 취소가 동작하지 않았다면 덱 박스 선택, 캐릭터 선택을 취소합니다.
		if (!bHandled)
		{
			CardStage->ResetSelectedDeckBox();
			CardPanelWidgetController->ResetSelectedCharacter();
		}
	}
	
	return MakeMouseUpReply();
}

void UCardPanelWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	FVector2D TargetUV;
	if (CardStage && TryGetCapturedCardStageUV(InMouseEvent, TargetUV))
	{
		CardStage->HandleCapturedMouseMove(TargetUV);
	}

	UpdateMouseInWorldSectionState(InMouseEvent);
}

void UCardPanelWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	DownPositions.Reset();

	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->UpdateMouseInWorldSection(false);
	}
}

void UCardPanelWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
	
	DownPositions.Reset();
}

FReply UCardPanelWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D TargetUV;
	if (CardStage)
	{
		const bool bIsMouseOnHandStage = TryGetCapturedCardStageUV(InMouseEvent, TargetUV);
		if (bIsMouseOnHandStage)
		{
			CardStage->HandleCapturedMouseMove(TargetUV);
		}
		else
		{
			CardStage->HandleCapturedMouseLeave();
		}
	}

	UpdateMouseInWorldSectionState(InMouseEvent);
	
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

bool UCardPanelWidget::TryGetCapturedCardStageUV(const FPointerEvent& InMouseEvent, FVector2D& OutUV) const
{
	if (!CapturedCardStage)
	{
		return false;
	}

	const FGeometry& ImageGeometry = CapturedCardStage->GetCachedGeometry();
	const FVector2D ImageSize = ImageGeometry.GetLocalSize();
	if (ImageSize.X <= UE_SMALL_NUMBER || ImageSize.Y <= UE_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D LocalPosition = ImageGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	OutUV = FVector2D(LocalPosition.X / ImageSize.X, LocalPosition.Y / ImageSize.Y);
	return OutUV.X >= 0.f && OutUV.X <= 1.f && OutUV.Y >= 0.f && OutUV.Y <= 1.f;
}

bool UCardPanelWidget::IsMouseInWorldSection(const FPointerEvent& InMouseEvent) const
{
	if (!WorldSection)
	{
		return false;
	}

	const FGeometry& SectionGeometry = WorldSection->GetCachedGeometry();
	const FVector2D SectionSize = SectionGeometry.GetLocalSize();
	if (SectionSize.X <= UE_SMALL_NUMBER || SectionSize.Y <= UE_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D LocalPosition = SectionGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
	return LocalPosition.X >= 0.f && LocalPosition.X <= SectionSize.X && LocalPosition.Y >= 0.f && LocalPosition.Y <= SectionSize.Y;
}

void UCardPanelWidget::UpdateMouseInWorldSectionState(const FPointerEvent& InMouseEvent) const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->UpdateMouseInWorldSection(IsMouseInWorldSection(InMouseEvent));
	}
}

void UCardPanelWidget::CreateCard(const FCardInitParams& CardInitParams) const
{
	if (CardStage)
	{
		CardStage->CreateCard(CardInitParams);
	}
}

void UCardPanelWidget::HandleKeyboardEvent(const int32 Number) const
{
	if (CardStage)
	{
		CardStage->HandleKeyboardEvent(Number);
	}
}

void UCardPanelWidget::OnSelectCardRequested(const int32 HandSlotIndex, ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle) const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->OnSelectCardRequested(HandSlotIndex, CardOwnerASC, AbilitySpecHandle);
	}
}

void UCardPanelWidget::OnCardSelected(const int32 HandSlotIndex) const
{
	if (CardStage)
	{
		CardStage->OnCardSelected(HandSlotIndex);
	}
}

void UCardPanelWidget::OnCancelSelectedCard() const
{
	if (CardStage)
	{
		CardStage->OnCancelSelectedCard();
	}
}

void UCardPanelWidget::OnResolveUseCard(const int32 HandSlotIndex, const bool bSuccess) const
{
	if (CardStage)
	{
		CardStage->OnResolveUseCard(HandSlotIndex, bSuccess);
	}
}

void UCardPanelWidget::StartViewCardDetail(const ACardActor* CardActor) const
{
	OnStartViewCardDetail.ExecuteIfBound(CardActor);
}

void UCardPanelWidget::NotifyDrawPhaseCompleted() const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->NotifyDrawPhaseCompleted();
	}
}

void UCardPanelWidget::StartResolvePlayerMoves() const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->StartResolvePlayerMoves();
	}
}

void UCardPanelWidget::RequestTurnEnd() const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->RequestTurnEnd();
	}
}

void UCardPanelWidget::RequestUseCard(ULetheAbilitySystemComponent* CardOwnerASC, const FGameplayAbilitySpecHandle& AbilitySpecHandle, const FGameplayTag& CardTag, const int32 HandSlotIndex) const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->RequestUseCard(CardOwnerASC, AbilitySpecHandle, CardTag, HandSlotIndex);
	}
}

void UCardPanelWidget::OnTurnEndButtonClicked()
{
	if (CardStage)
	{
		CardStage->OnTurnEndButtonClicked();
	}
}

FReply UCardPanelWidget::MakeMouseUpReply() const
{
	return DownPositions.IsEmpty() ? FReply::Handled().ReleaseMouseCapture() : FReply::Unhandled().ReleaseMouseCapture();
}

bool UCardPanelWidget::CanRouteMouseInput() const
{
#if WITH_EDITOR
	if (const UWorld* World = GetWorld())
	{
		if (const UGameViewportClient* GameViewportClient = World->GetGameViewport())
		{
			return !GameViewportClient->IsSimulateInEditorViewport();
		}
	}
#endif
	return true;
}
