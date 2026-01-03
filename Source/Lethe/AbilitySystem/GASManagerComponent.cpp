// Copyright JETBLU, Inc. All Rights Reserved.

#include "GASManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"

UGASManagerComponent::UGASManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UGASManagerComponent::SetAbilitySystemComponent(UAbilitySystemComponent* InAbilitySystemComponent)
{
	AbilitySystemComponent = InAbilitySystemComponent;
}

void UGASManagerComponent::SetAttributeSet(UAttributeSet* InAttributeSet)
{
	AttributeSet = InAttributeSet;
}

UAbilitySystemComponent* UGASManagerComponent::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void UGASManagerComponent::InitAbilityActorInfo()
{
	AActor* OwnerActor = GetOwner();
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(OwnerActor);
	
	AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
	AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, OwnerActor);
}

void UGASManagerComponent::AddCharacterStartupAbilities() const
{
	ULetheAbilitySystemComponent* ASC = CastChecked<ULetheAbilitySystemComponent>(AbilitySystemComponent);
	ASC->AddCharacterAbilities(StartupAbilities);
	ASC->AddCharacterAbilitiesWithActive(StartupPassiveAbilities);
}

void UGASManagerComponent::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const
{
	check(IsValid(AbilitySystemComponent));
	check(GameplayEffectClass);

	const FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
}
