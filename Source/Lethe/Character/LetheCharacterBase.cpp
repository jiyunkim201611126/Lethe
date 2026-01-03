// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheCharacterBase.h"

#include "AbilitySystemComponent.h"
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
}

UAbilitySystemComponent* ALetheCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ALetheCharacterBase::Die()
{
}

void ALetheCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	InitAbilityActorInfo();
	GASManagerComponent->AddCharacterStartupAbilities();
}

void ALetheCharacterBase::InitAbilityActorInfo() const
{
	GASManagerComponent->SetAbilitySystemComponent(AbilitySystemComponent);
	GASManagerComponent->SetAttributeSet(AttributeSet);
	GASManagerComponent->InitAbilityActorInfo();
}
