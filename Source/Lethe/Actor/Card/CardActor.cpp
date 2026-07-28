// Copyright JETBLU, Inc. All Rights Reserved.

#include "CardActor.h"

#include "Lethe/UI/Battle/Card/CardPanelWidgetController.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/Data/Card/CardDefinitionData.h"
#include "Lethe/Data/Card/CardViewData.h"
#include "Lethe/UI/Battle/DeckEditing/CardWidgetInitContext.h"

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
	AbilitySpecHandle = InitParams.AbilitySpecHandle;

	if (InitParams.CardDefinition)
	{
		CardNameText = InitParams.CardDefinition->CardNameText;
		CardTexture = InitParams.CardDefinition->CardTexture;

		if (InitParams.CardViewData)
		{
			CardTypeColor = InitParams.CardViewData->GetCardTypeColor(InitParams.CardDefinition->CardTypeTag);
		}
	}

	ApplyCardVisuals();
}

void ACardActor::CreateDynamicMaterialInstances()
{
	if (CardMesh)
	{
		if (UMaterialInterface* MaterialInterface = CardMesh->GetMaterial(2))
		{
			IllustrationMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, MaterialInterface);
			CardMesh->SetMaterial(2, IllustrationMaterialInstance);
		}
		if (UMaterialInterface* MaterialInterface = CardMesh->GetMaterial(4))
		{
			LeftTagMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, MaterialInterface);
			CardMesh->SetMaterial(4, LeftTagMaterialInstance);
		}
		if (UMaterialInterface* MaterialInterface = CardMesh->GetMaterial(5))
		{
			RightTagMaterialInstance = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, MaterialInterface);
			CardMesh->SetMaterial(5, RightTagMaterialInstance);
		}
	}
}

void ACardActor::ApplyCardVisuals() const
{
	if (!IllustrationMaterialInstance || !CardTexture)
	{
		return;
	}

	IllustrationMaterialInstance->SetTextureParameterValue(CardTextureParamName, Cast<UTexture>(CardTexture));
	LeftTagMaterialInstance->SetVectorParameterValue(FrameColorParamName, CardTypeColor);
}

void ACardActor::SetCardContainer(const ECardContainer InCardContainer)
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
	case ECardContainer::Hands:
		break;
	case ECardContainer::Selected:
		ToggleHighlightOutline(true);
		break;
	case ECardContainer::Graves:
		break;
	}
}

void ACardActor::ToggleHighlightOutline(const bool bHighlightOn) const
{
	CardOutlineMesh->SetHiddenInGame(!bHighlightOn);
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
	case ECardContainer::Hands:
		return GetCardActionWhenHandState(InMouseEvent);
	default:
		break;
	}
	return ECardAction::None;
}

ECardAction ACardActor::GetCardActionWhenDeckState(const ECardMouseEvent InMouseEvent) const
{
	switch (InMouseEvent)
	{
	case ECardMouseEvent::LeftMouseButtonUp:
		return ECardAction::Select;
	case ECardMouseEvent::RightMouseButtonUp:
		return ECardAction::ViewDetail;
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
		return ECardAction::Select;
	case ECardMouseEvent::RightMouseButtonUp:
		return ECardAction::ViewDetail;
	default:
		break;
	}
	return ECardAction::None;
}

void ACardActor::MakeCardWidgetInitContext(UCardWidgetInitContext*& OutContext) const
{
	OutContext->CardNameText = CardNameText;
	OutContext->SavedCard = SavedCard;
	OutContext->CardTexture = CardTexture;
	OutContext->CardTypeColor = CardTypeColor;
}

FGameplayAbilitySpecHandle ACardActor::GetAbilitySpecHandle() const
{
	return AbilitySpecHandle;
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
