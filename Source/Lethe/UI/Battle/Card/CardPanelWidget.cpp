// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardPanelWidgetController.h"
#include "ViewCardDetailWidget.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Lethe/Actor/Card/CardStage.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
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
		CardPanelWidgetController->OnPhaseStateChangedDelegate.RemoveAll(this);
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

	Super::NativeDestruct();
}

void UCardPanelWidget::WidgetControllerSet_Implementation()
{
	if (!CardPanelWidgetController)
	{
		CardPanelWidgetController = Cast<UCardPanelWidgetController>(WidgetController);
	}

	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->OnAbilityUpdatedDelegate.BindUObject(this, &ThisClass::CreateCard);
		CardPanelWidgetController->OnAbilitySystemReferencesUpdatedDelegate.BindUObject(this, &ThisClass::TryInitializeCardStage);
		CardPanelWidgetController->OnPhaseStateChangedDelegate.AddUObject(this, &ThisClass::OnPhaseStateChanged);
		CardPanelWidgetController->OnCardSelectCanceledDelegate.BindUObject(this, &ThisClass::OnCancelSelectedCard);
		CardPanelWidgetController->OnUseCardResolvedDelegate.BindUObject(this, &ThisClass::OnResolveUseCard);

		ViewCardDetail->SetWidgetController(WidgetController);

		if (CardStageClass && !CardStage)
		{
			const FVector StageLocation = FVector(0.f, 0.f, -3000.f);
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			CardStage = GetWorld()->SpawnActor<ACardStage>(CardStageClass, StageLocation, FRotator::ZeroRotator, SpawnParams);
			if (CardStage)
			{
				CardStage->OnViewCardDetailRequested.BindUObject(this, &ThisClass::StartViewCardDetail);
				CardStage->OnSelectCardRequested.BindUObject(this, &ThisClass::SetCardSelected);
				CardStage->OnGoPlayerTurnPhaseRequested.BindUObject(this, &ThisClass::GoPlayerTurnPhase);
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
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton && InMouseEvent.GetEffectingButton() != EKeys::RightMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	// 마우스 클릭은 모두 위젯을 거쳐 처리되므로 일단 캡쳐합니다.
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCardPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!CardStage)
	{
		return FReply::Handled().ReleaseMouseCapture();
	}
	
	FVector2D TargetUV;
	const bool bIsMouseInCardStage = TryGetCapturedCardStageUV(InMouseEvent, TargetUV);
	
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (bIsMouseInCardStage)
		{
			if (CardStage->HandleLeftMouseButtonClickedInCardStageSection(TargetUV))
			{
				return FReply::Handled().ReleaseMouseCapture();
			}
		}
		if (IsMouseInWorldSection(InMouseEvent))
		{
			// 카드 선택도 사용도 하지 않았으며 마우스는 월드 섹션에 있는 경우, PlayerController에게 좌클릭 입력을 내려줍니다.
			const bool bHandled = CardStage->HandleLeftMouseButtonClickedInWorldSection();
			if (!bHandled)
			{
				if (ALethePlayerController* LethePlayerController = GetOwningPlayer<ALethePlayerController>())
				{
					LethePlayerController->HandleLeftMouseButtonClickedOnWorld();
				}
			}
		}
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		bool bHandled = false;
		if (bIsMouseInCardStage)
		{
			bHandled = CardStage->HandleViewDetail(TargetUV);
		}

		if (!bHandled)
		{
			CardStage->CancelSelect();
			if (ALethePlayerController* LethePlayerController = GetOwningPlayer<ALethePlayerController>())
			{
				LethePlayerController->ResetSelectedCharacter();
			}
		}
	}
	
	return FReply::Handled().ReleaseMouseCapture();
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
	if (ALethePlayerController* LethePlayerController = GetOwningPlayer<ALethePlayerController>())
	{
		LethePlayerController->SetMouseOnWorldSection(IsMouseInWorldSection(InMouseEvent));
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

void UCardPanelWidget::OnCancelSelectedCard() const
{
	if (CardStage)
	{
		CardStage->HandleCancelSelectedCard();
	}
}

void UCardPanelWidget::OnResolveUseCard(const int32 HandIndex, const bool bSuccess) const
{
	if (CardStage)
	{
		CardStage->HandleResolveUseCard(HandIndex, bSuccess);
	}
}

void UCardPanelWidget::StartViewCardDetail(const ACardActor* CardActor) const
{
	ViewCardDetail->StartViewDetail(CardActor);
}

bool UCardPanelWidget::SetCardSelected(const bool bCardSelected, ULetheAbilitySystemComponent* OwnerASC, const FGameplayTag& CardTag) const
{
	return CardPanelWidgetController && CardPanelWidgetController->SetCardSelected(bCardSelected, OwnerASC, CardTag);
}

void UCardPanelWidget::GoPlayerTurnPhase() const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->GoPlayerTurnPhase();
	}
}

void UCardPanelWidget::StartResolvePlayerMoves() const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->StartResolvePlayerMoves();
	}
}

bool UCardPanelWidget::RequestTurnEnd() const
{
	return CardPanelWidgetController && CardPanelWidgetController->RequestTurnEnd();
}

void UCardPanelWidget::RequestUseCard(ULetheAbilitySystemComponent* OwnerASC, const FSavedCard& SavedCard, const int32 HandIndex) const
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->RequestUseCard(OwnerASC, SavedCard, HandIndex);
	}
}

void UCardPanelWidget::OnTurnEndButtonClicked()
{
	if (CardStage)
	{
		CardStage->HandleTurnEndButtonClicked();
	}
}

void UCardPanelWidget::OnPhaseStateChanged(const EPhaseState OldState, const EPhaseState NewState) const
{
	if (CardStage)
	{
		CardStage->HandlePhaseStateChanged(OldState, NewState);
	}
}
