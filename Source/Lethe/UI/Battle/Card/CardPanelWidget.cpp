// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardPanelWidget.h"

#include "CardPanelWidgetController.h"
#include "CardUseSectionWidget.h"
#include "ViewCardDetailWidget.h"
#include "Components/Button.h"
#include "InputCoreTypes.h"
#include "Lethe/Actor/Card/CardStage.h"
#include "Lethe/UI/Core/LetheImage.h"

void UCardPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();

	TurnEndButton->OnClicked.AddDynamic(this, &ThisClass::OnTurnEndButtonClicked);
	CardUseSection->OnMouseButtonDown.BindUObject(this, &ThisClass::OnMouseButtonDownInCardUseSection);
	CardUseSection->OnMouseButtonUp.BindUObject(this, &ThisClass::OnMouseButtonUpInCardUseSection);
}

void UCardPanelWidget::NativeDestruct()
{
	if (CardPanelWidgetController)
	{
		CardPanelWidgetController->OnAbilityUpdatedDelegate.Unbind();
		CardPanelWidgetController->OnAbilitySystemReferencesUpdatedDelegate.Unbind();
		CardPanelWidgetController->OnPhaseStateChangedDelegate.RemoveAll(this);
		CardPanelWidgetController->OnNumberKeyPressedDelegate.Unbind();
		CardPanelWidgetController->OnCardSelectCanceledDelegate.Unbind();
		CardPanelWidgetController->OnUseCardResolvedDelegate.Unbind();
	}

	if (TurnEndButton)
	{
		TurnEndButton->OnClicked.RemoveDynamic(this, &ThisClass::OnTurnEndButtonClicked);
	}
	if (CardUseSection)
	{
		CardUseSection->OnMouseButtonDown.Unbind();
		CardUseSection->OnMouseButtonUp.Unbind();
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
		CardPanelWidgetController->OnNumberKeyPressedDelegate.BindUObject(this, &ThisClass::OnKeyboardEvent);
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

FReply UCardPanelWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (CardStage && InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		CardStage->CancelSelectedCard();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCardPanelWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D TargetUV;
	if (TryGetCapturedCardStageUV(InMouseEvent, TargetUV))
	{
		if (CardStage && CardStage->HandleCapturedMouseButtonDown(TargetUV, InMouseEvent.GetEffectingButton()))
		{
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UCardPanelWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FVector2D TargetUV;
	if (TryGetCapturedCardStageUV(InMouseEvent, TargetUV))
	{
		if (CardStage && CardStage->HandleCapturedMouseButtonUp(TargetUV, InMouseEvent.GetEffectingButton()))
		{
			return FReply::Handled().ReleaseMouseCapture();
		}
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCardPanelWidget::NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent)
{
	if (CardStage)
	{
		CardStage->HandleCapturedMouseCaptureLost();
	}

	Super::NativeOnMouseCaptureLost(CaptureLostEvent);
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

void UCardPanelWidget::CreateCard(const FCardInitParams& CardInitParams) const
{
	if (CardStage)
	{
		CardStage->CreateCard(CardInitParams);
	}
}

void UCardPanelWidget::OnKeyboardEvent(const int32 Number) const
{
	if (CardStage)
	{
		CardStage->HandleKeyboardEvent(Number);
	}
}

bool UCardPanelWidget::OnMouseButtonDownInCardUseSection() const
{
	return CardStage && CardStage->HandleMouseButtonDownInCardUseSection();
}

bool UCardPanelWidget::OnMouseButtonUpInCardUseSection() const
{
	return CardStage && CardStage->HandleMouseButtonUpInCardUseSection();
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
