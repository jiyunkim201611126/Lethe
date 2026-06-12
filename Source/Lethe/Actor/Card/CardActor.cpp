// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardActor.h"

#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardViewData.h"

ACardActor::ACardActor()
{
	PrimaryActorTick.bCanEverTick = false;

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
	CardOutlineMesh->SetHiddenInGame(true);
}

void ACardActor::BeginPlay()
{
	Super::BeginPlay();

	CreateDynamicMaterialInstances();
}

void ACardActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	OnCardMouseEventDelegate.Unbind();
	Super::EndPlay(EndPlayReason);
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

ECardAction ACardActor::GetCardActionForMouseEvent(const ECardMouseEvent InMouseEvent) const
{
	switch (CurrentCardContainer)
	{
	case ECardContainer::Deck:
		return GetCardActionWhenDeckState(InMouseEvent);
	case ECardContainer::Hand:
		return GetCardActionWhenHandState(InMouseEvent);
	default:
		break;
	}
	return ECardAction::None;
}

void ACardActor::CreateDynamicMaterialInstances()
{
	if (CardMesh && CardMesh->GetMaterial(0))
	{
		IllustrationMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CardMesh->GetMaterial(2));
		CardMesh->SetMaterial(2, IllustrationMaterialInstance);
		LeftTagMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CardMesh->GetMaterial(4));
		CardMesh->SetMaterial(4, LeftTagMaterialInstance);
		RightTagMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CardMesh->GetMaterial(5));
		CardMesh->SetMaterial(5, RightTagMaterialInstance);
	}
}

void ACardActor::ApplyCardVisuals() const
{
	if (!IllustrationMaterialInstance || !CardImage)
	{
		return;
	}

	IllustrationMaterialInstance->SetTextureParameterValue(CardTextureParamName, Cast<UTexture>(CardImage));
	LeftTagMaterialInstance->SetVectorParameterValue(FrameColorParamName, CardTypeColor);
}

void ACardActor::ToggleHighlightOutline(const bool bHighlightOn) const
{
	CardOutlineMesh->SetHiddenInGame(!bHighlightOn);
}

ECardAction ACardActor::GetCardActionWhenDeckState(const ECardMouseEvent InMouseEvent) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::LeftMouseButtonUp:
		return ECardAction::Draw;
	default:
		break;
	}
	return ECardAction::None;
}

ECardAction ACardActor::GetCardActionWhenHandState(const ECardMouseEvent InMouseEvent) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::LeftMouseButtonUp:
		return ECardAction::Selected;
	case ECardMouseEvent::RightMouseButtonUp:
		return ECardAction::ViewDetail;
	default:
		break;
	}
	return ECardAction::None;
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
