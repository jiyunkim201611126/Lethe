// Copyright JETBLU, Inc. All Rights Reserved.

#include "GASManagerComponent.h"

#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Lethe/AbilitySystem/LetheAbilitySystemComponent.h"
#include "Lethe/UI/HUD/LetheHUD.h"

UGASManagerComponent::UGASManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
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
	
	AbilitySystemComponent->InitAbilityActorInfo(OwnerActor, OwnerActor);
	ApplyEffectToSelf(DefaultAttributes, 1.f);

	// 플레이어블 캐릭터인 경우만 HUD쪽으로 넘겨줍니다.
	if (TeamSide == ETeamSide::Player)
	{
		// PlayerController가 빙의하는 캐릭터가 아니기 때문에 라이브러리 함수로 가져옵니다.
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			if (ALetheHUD* LetheHUD = PlayerController->GetHUD<ALetheHUD>())
			{
				LetheHUD->InitHUD(PlayerController, GetPawn<APawn>()->GetPlayerState(), AbilitySystemComponent, AttributeSet);
			}
		}
	}
	
	AddCharacterAbilities(TestAbilities);
}

void UGASManagerComponent::AddCharacterAbilities(const TArray<TSubclassOf<UGameplayAbility>>& InAbilities) const
{
	ULetheAbilitySystemComponent* ASC = CastChecked<ULetheAbilitySystemComponent>(AbilitySystemComponent);
	ASC->AddCharacterAbilities(InAbilities);
}

void UGASManagerComponent::ApplyEffectToSelf(const TSubclassOf<UGameplayEffect>& GameplayEffectClass, const float Level) const
{
	check(IsValid(AbilitySystemComponent));
	check(GameplayEffectClass);

	const FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffectClass, Level, ContextHandle);
	AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
}
