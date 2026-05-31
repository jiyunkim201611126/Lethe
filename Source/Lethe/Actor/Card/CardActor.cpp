// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardActor.h"

#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture.h"
#include "InputCoreTypes.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/CharacterDefinitionData.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardViewData.h"

ACardActor::ACardActor()
{
	PrimaryActorTick.bCanEverTick = true;

	CardRoot = CreateDefaultSubobject<USceneComponent>(TEXT("CardRoot"));
	SetRootComponent(CardRoot);

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetupAttachment(CardRoot);
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Card, ECR_Block);
	InteractionBox->SetGenerateOverlapEvents(false);

	CardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardFrontMesh"));
	CardMesh->SetupAttachment(CardRoot);
	CardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CardOutlineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CardOutlineMesh"));
	CardOutlineMesh->SetupAttachment(CardRoot);
	CardOutlineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ACardActor::BeginPlay()
{
	Super::BeginPlay();

	CreateDynamicMaterialInstances();

	FOnTimelineFloat OnUpdateFunction;
	OnUpdateFunction.BindDynamic(this, &ThisClass::OnUpdatedTimeline);
	MovementTimeline.AddInterpFloat(MovementCurve, OnUpdateFunction);

	FOnTimelineEvent OnFinishedFunction;
	OnFinishedFunction.BindDynamic(this, &ThisClass::OnFinishedTimeline);
	MovementTimeline.SetTimelineFinishedFunc(OnFinishedFunction);

	StartTransform = GetActorTransform();
	TargetTransform = StartTransform;
}

void ACardActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnCardMouseEventDelegate.Unbind();
	Super::EndPlay(EndPlayReason);
}

void ACardActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bShouldMove)
	{
		MovementTimeline.TickTimeline(DeltaSeconds);
	}
}

void ACardActor::SetCardInfo(const FCardInitParams& InitParams)
{
	OwnerASC = InitParams.OwnerASC;
	SavedCard = InitParams.SavedCard;

	if (InitParams.CardDefinition)
	{
		CardNameText = InitParams.CardDefinition->CardNameText;
		CardImage = InitParams.CardDefinition->CardTexture;

		if (InitParams.CardViewData)
		{
			CardTypeColor = InitParams.CardViewData->GetCardTypeColor(InitParams.CardDefinition->CardTypeTag);
		}
	}

	if (InitParams.CharacterDefinitionData)
	{
		CharacterColor = FLinearColor(InitParams.CharacterDefinitionData->PersonalColor);
	}

	ApplyCardVisuals();
}

void ACardActor::MakeViewDetailData(FViewDetailData& OutData) const
{
	OutData.CardNameText = CardNameText;
	OutData.CardImage = CardImage;
	OutData.CardTypeColor = CardTypeColor;
}

void ACardActor::SetCardContainer(const ECardContainer InCardContainer, const bool bShouldSkipAnimation)
{
	if (CurrentCardContainer == InCardContainer)
	{
		return;
	}

	ToggleHighlightOutline(false);

	CurrentCardContainer = InCardContainer;
	switch (CurrentCardContainer)
	{
	case ECardContainer::Deck:
		break;
	case ECardContainer::Hand:
		bMouseHovered = false;
		if (!bShouldSkipAnimation)
		{
			StartBlockHandHoverTimer();
		}
		break;
	case ECardContainer::Selected:
		ToggleHighlightOutline(true);
		break;
	case ECardContainer::Grave:
		break;
	}
}

void ACardActor::HandleCardMouseEvent(const ECardMouseEvent InMouseEvent)
{
	OnCardMouseEventDelegate.ExecuteIfBound(this, GetCardActionForMouseEvent(InMouseEvent));
}

void ACardActor::SetTargetTransform(const FTransform& InTransform)
{
	StartTransform = GetActorTransform();
	TargetTransform = InTransform;

	if (!MovementCurve)
	{
		FinishMovementImmediately();
		return;
	}

	bShouldMove = true;
	MovementTimeline.PlayFromStart();
}

void ACardActor::MouseHovered(const bool bInHovered)
{
	if (bMouseHovered == bInHovered)
	{
		return;
	}

	bMouseHovered = bInHovered;
	SetTargetTransform(TargetTransform);
}

ECardAction ACardActor::GetCardActionForMouseEvent(const ECardMouseEvent InMouseEvent) const
{
	switch (CurrentCardContainer)
	{
	case ECardContainer::Deck:
		return GetCardActionWhenDeckState(InMouseEvent);
	case ECardContainer::Hand:
		return GetCardActionWhenHandState(InMouseEvent);
	case ECardContainer::Selected:
		return GetCardActionWhenSelectedState(InMouseEvent);
	default:
		break;
	}
	return ECardAction::None;
}

void ACardActor::OnUpdatedTimeline(const float InValue)
{
	const FVector LerpedLocation = FMath::Lerp(StartTransform.GetLocation(), GetTargetTransformWithHoverOffset().GetLocation(), InValue);
	const FQuat LerpedRotation = FQuat::Slerp(StartTransform.GetRotation(), TargetTransform.GetRotation(), InValue);
	const FVector LerpedScale = FMath::Lerp(StartTransform.GetScale3D(), TargetTransform.GetScale3D(), InValue);

	SetActorTransform(FTransform(LerpedRotation, LerpedLocation, LerpedScale));
}

void ACardActor::OnFinishedTimeline()
{
	bShouldMove = false;
	StartTransform = TargetTransform;
	SetActorTransform(GetTargetTransformWithHoverOffset());
}

void ACardActor::CreateDynamicMaterialInstances()
{
	if (CardMesh && CardMesh->GetMaterial(0))
	{
		CardMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CardMesh->GetMaterial(0));
		CardMesh->SetMaterial(0, CardMaterialInstance);
	}
}

void ACardActor::ApplyCardVisuals() const
{
	if (!CardMaterialInstance || !CardImage)
	{
		return;
	}

	CardMaterialInstance->SetTextureParameterValue(CardTextureParamName, Cast<UTexture>(CardImage));
	CardMaterialInstance->SetVectorParameterValue(TypeFrameColorParamName, CardTypeColor);
	CardMaterialInstance->SetVectorParameterValue(CharacterColorParamName, CharacterColor);
}

void ACardActor::ToggleHighlightOutline(const bool bHighlightOn) const
{
	CardOutlineMesh->SetVisibility(!bHighlightOn);
}

void ACardActor::StartBlockHandHoverTimer()
{
	bBlockHandHover = true;

	FTimerHandle TimerHandle;
	TWeakObjectPtr<ACardActor> WeakThis = this;
	GetWorldTimerManager().SetTimer(TimerHandle, [WeakThis]()
	{
		if (WeakThis.IsValid())
		{
			WeakThis->bBlockHandHover = false;
		}
	}, 0.5f, false);
}

ECardAction ACardActor::GetCardActionWhenDeckState(const ECardMouseEvent InMouseEvent) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::MouseEnter:
		return ECardAction::DeckHovered;
	case ECardMouseEvent::MouseLeave:
		return ECardAction::DeckUnhovered;
	case ECardMouseEvent::LeftMouseButtonUp:
		return ECardAction::Draw;
	case ECardMouseEvent::MouseCaptureLost:
		return ECardAction::DeckUnhovered;
	default:
		break;
	}
	return ECardAction::None;
}

ECardAction ACardActor::GetCardActionWhenHandState(const ECardMouseEvent InMouseEvent) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::MouseEnter:
		if (!bBlockHandHover)
		{
			return ECardAction::HandHovered;
		}
		break;
	case ECardMouseEvent::MouseLeave:
		if (!bBlockHandHover)
		{
			return ECardAction::HandUnhovered;
		}
		break;
	case ECardMouseEvent::LeftMouseButtonUp:
		return ECardAction::Selected;
	case ECardMouseEvent::RightMouseButtonUp:
		return ECardAction::ViewDetail;
	case ECardMouseEvent::MouseCaptureLost:
		return ECardAction::HandUnhovered;
	default:
		break;
	}
	return ECardAction::None;
}

ECardAction ACardActor::GetCardActionWhenSelectedState(const ECardMouseEvent InMouseEvent) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::MouseEnter:
		if (!bBlockHandHover)
		{
			return ECardAction::HandHovered;
		}
		break;
	case ECardMouseEvent::MouseLeave:
		if (!bBlockHandHover)
		{
			return ECardAction::HandUnhovered;
		}
		break;
	default:
		break;
	}
	return ECardAction::None;
}

FTransform ACardActor::GetTargetTransformWithHoverOffset() const
{
	FTransform TransformWithHoverOffset = TargetTransform;
	if (bMouseHovered)
	{
		TransformWithHoverOffset.AddToTranslation(TargetTransform.GetRotation().RotateVector(HoveredLocalOffset));
	}
	return TransformWithHoverOffset;
}

void ACardActor::FinishMovementImmediately()
{
	bShouldMove = false;
	StartTransform = TargetTransform;
	SetActorTransform(GetTargetTransformWithHoverOffset());
}

FGameplayTag ACardActor::GetCardTag() const
{
	return SavedCard.CardTag;
}

const FSavedCard& ACardActor::GetSavedCard() const
{
	return SavedCard;
}

ECardContainer ACardActor::GetCurrentCardContainer() const
{
	return CurrentCardContainer;
}

ULetheAbilitySystemComponent* ACardActor::GetOwnerASC() const
{
	if (OwnerASC.IsValid())
	{
		return OwnerASC.Get();
	}
	return nullptr;
}
