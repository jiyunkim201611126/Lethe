// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCharacterBase.h"

#include "AbilitySystemComponent.h"
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
}

void ALetheCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetAbilitySystemComponent(AbilitySystemComponent);
	GASManagerComponent->SetAttributeSet(AttributeSet);
	GASManagerComponent->InitAbilityActorInfo(AttributeWidgetComponent->GetWidget());
	GASManagerComponent->AddCharacterAbilities(StartAbilities);
}
