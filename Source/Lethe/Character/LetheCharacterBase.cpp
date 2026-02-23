// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCharacterBase.h"

#include "AbilitySystemComponent.h"
#include "LethePawn.h"
#include "Components/WidgetComponent.h"
#include "Lethe/AbilitySystem/GASManagerComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAttributeSet.h"

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

	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->OnPossessedPawnChanged.AddDynamic(this, &ThisClass::BindCameraHeightChanged);
	
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			if (ALethePawn* PlayerPawn = Cast<ALethePawn>(Pawn))
			{
				BindCameraHeightChanged(nullptr, PlayerPawn);
				OnCameraHeightChanged(PlayerPawn->GetAttributeWidgetSize());
			}
		}
	}
}

void ALetheCharacterBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
	{
		PlayerController->OnPossessedPawnChanged.RemoveDynamic(this, &ThisClass::BindCameraHeightChanged);
	
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			UnbindCameraHeightChanged(Pawn);
		}
	}
	
	Super::EndPlay(EndPlayReason);
}

void ALetheCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetAbilitySystemComponent(AbilitySystemComponent);
	GASManagerComponent->SetAttributeSet(AttributeSet);
	GASManagerComponent->InitAbilityActorInfo(AttributeWidgetComponent->GetWidget());
}

void ALetheCharacterBase::BindCameraHeightChanged(APawn* OldPawn, APawn* NewPawn)
{
	UnbindCameraHeightChanged(OldPawn);
	if (ALethePawn* NewPlayerPawn = Cast<ALethePawn>(NewPawn))
	{
		OnCameraHeightChangedDelegateHandle = NewPlayerPawn->OnCameraHeightChanged.AddUObject(this, &ThisClass::OnCameraHeightChanged);
	}
}

void ALetheCharacterBase::UnbindCameraHeightChanged(APawn* OldPawn) const
{
	if (ALethePawn* OldPlayerPawn = Cast<ALethePawn>(OldPawn))
	{
		OldPlayerPawn->OnCameraHeightChanged.Remove(OnCameraHeightChangedDelegateHandle);
	}
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
