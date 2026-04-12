// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "LethePawn.h"
#include "Component/GASManagerComponent.h"
#include "Components/WidgetComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Controller/PlayerController/LethePlayerController.h"
#include "Lethe/UI/Framework/LetheUserWidget.h"

ALetheCharacterBase::ALetheCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	GASManagerComponent = CreateDefaultSubobject<UGASManagerComponent>(TEXT("GASManagerComponent"));
	AbilitySystemComponent = CreateDefaultSubobject<ULetheAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<ULetheAttributeSet>(TEXT("AttributeSet"));

	CharacterStatusWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("CharacterStatusWidgetComponent"));
	CharacterStatusWidgetComponent->SetupAttachment(RootComponent);
}

UAbilitySystemComponent* ALetheCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALetheCharacterBase::SetLocationOnTile(FVector InTileLocation)
{
	// 캐릭터 절반 높이만큼 위로 올려줍니다.
	InTileLocation.Z += GetDefaultHalfHeight();
	SetActorLocation(InTileLocation);
}

int32 ALetheCharacterBase::GetMoveDistance() const
{
	return FMath::Max(0, FMath::FloorToInt(AttributeSet->GetMoveDistance()));
}

int32 ALetheCharacterBase::GetMaxMoveDistance() const
{
	return FMath::Max(0, FMath::FloorToInt(AttributeSet->GetMaxMoveDistance()));
}

void ALetheCharacterBase::OnDamageTaken()
{
	// TODO: 피격 애니메이션 재생 등
}

void ALetheCharacterBase::Die()
{
	GASManagerComponent->OnDied();
}

void ALetheCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	TArray<UWidgetComponent*> WidgetComponents;
	GetComponents(UWidgetComponent::StaticClass(), WidgetComponents);
	WidgetComponents.RemoveAllSwap([](const UWidgetComponent* Component)
	{
		if (const UUserWidget* Widget = Component->GetWidget())
		{
			return !Widget->IsA(ULetheUserWidget::StaticClass());
		}
		return true;
	});
	AttributeWidgetComponents = MoveTemp(WidgetComponents);

	InitAbilityActorInfo();

	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->OnCameraHeightChangedDelegate.AddUObject(this, &ALetheCharacterBase::OnCameraHeightChanged);
		if (const ALethePawn* LethePawn = Cast<ALethePawn>(PlayerController->GetPawn()))
		{
			OnCameraHeightChanged(LethePawn->GetAttributeWidgetSize());
		}
	}
}

void ALetheCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		PlayerController->OnCameraHeightChangedDelegate.RemoveAll(this);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheCharacterBase::HighlightActorByMouse_Implementation()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(OutlineColor);
}

void ALetheCharacterBase::HighlightActorTransparentByMouse_Implementation()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(OutlineColorTransparent);
}

void ALetheCharacterBase::UnhighlightActorByMouse_Implementation()
{
	GetMesh()->SetRenderCustomDepth(false);
}

void ALetheCharacterBase::HighlightActorByAbility_Implementation(const int32 InOutlineColor)
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(OutlineColor);
}

void ALetheCharacterBase::UnhighlightActorByAbility_Implementation()
{
	GetMesh()->SetRenderCustomDepth(false);
}

void ALetheCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetAbilitySystemComponent(AbilitySystemComponent);
	GASManagerComponent->SetAttributeSet(AttributeSet);

	TArray<UUserWidget*> AttributeWidgets;
	for (const UWidgetComponent* WidgetComponent : AttributeWidgetComponents)
	{
		AttributeWidgets.Emplace(WidgetComponent->GetWidget());
	}
	GASManagerComponent->InitAbilityActorInfo(AttributeWidgets);
}

void ALetheCharacterBase::OnCameraHeightChanged(const float InWidgetSize) const
{
	for (const UWidgetComponent* WidgetComponent : AttributeWidgetComponents)
	{
		if (UUserWidget* Widget = WidgetComponent->GetWidget())
		{
			Widget->SetRenderScale(FVector2D(InWidgetSize));
		}
	}
}
