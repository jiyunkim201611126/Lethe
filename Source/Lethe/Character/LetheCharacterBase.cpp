// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "LethePawn.h"
#include "Components/WidgetComponent.h"
#include "Lethe/AbilitySystem/GASManagerComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"
#include "Lethe/Player/PlayerController/LethePlayerController.h"

ALetheCharacterBase::ALetheCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	GASManagerComponent = CreateDefaultSubobject<UGASManagerComponent>(TEXT("GASManagerComponent"));
	AbilitySystemComponent = CreateDefaultSubobject<ULetheAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AttributeSet = CreateDefaultSubobject<ULetheAttributeSet>(TEXT("AttributeSet"));

	AttributeWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("AttributeWidgetComponent"));
	AttributeWidgetComponent->SetupAttachment(RootComponent);
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

void ALetheCharacterBase::Die()
{
}

void ALetheCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	InitAbilityActorInfo();

	if (ALethePlayerController* PlayerController = Cast<ALethePlayerController>(GetWorld()->GetFirstPlayerController()))
	{
		OnCameraHeightChangedDelegateHandle = PlayerController->OnCameraHeightChangedDelegate.AddUObject(this, &ALetheCharacterBase::OnCameraHeightChanged);
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
		PlayerController->OnCameraHeightChangedDelegate.Remove(OnCameraHeightChangedDelegateHandle);
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetAbilitySystemComponent(AbilitySystemComponent);
	GASManagerComponent->SetAttributeSet(AttributeSet);
	GASManagerComponent->InitAbilityActorInfo(AttributeWidgetComponent->GetWidget());
}

void ALetheCharacterBase::OnCameraHeightChanged(const float InWidgetSize) const
{
	if (AttributeWidgetComponent)
	{
		if (UUserWidget* AttributeWidget = AttributeWidgetComponent->GetWidget())
		{
			AttributeWidget->SetRenderScale(FVector2D(InWidgetSize));
		}
	}
}
