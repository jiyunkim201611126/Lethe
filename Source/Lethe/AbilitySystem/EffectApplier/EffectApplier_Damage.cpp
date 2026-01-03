// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectApplier_Damage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"

void UEffectApplier_Damage::ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor)
{
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(TargetActor);
	const FLetheGameplayTags& GameplayTags = FLetheGameplayTags::Get();
	if (AbilitySystemInterface && AbilitySystemInterface->GetAbilitySystemComponent() && AbilitySystemInterface->GetAbilitySystemComponent()->HasMatchingGameplayTag(GameplayTags.CharacterState_Dead))
	{
		// GameplayEffect를 적용할 대상이 이미 사망한 경우 로직을 중단합니다.
		EffectContextHandle.Clear();
		return;
	}
	
	CauseDamage(OwningAbility, TargetActor, MakeDamageSpecHandle(OwningAbility));
}

TArray<FGameplayEffectSpecHandle> UEffectApplier_Damage::MakeDamageSpecHandle(const UGameplayAbility* OwningAbility)
{
	const UAbilitySystemComponent* ASC = OwningAbility->GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return TArray<FGameplayEffectSpecHandle>();
	}
	
	MakeEffectContextHandle(OwningAbility);
	
	TArray<FGameplayEffectSpecHandle> DamageSpecs;
	for (TPair<FGameplayTag, FScalableFloat>& Pair : DamageTypes)
	{
		if (Pair.Value.IsValid())
		{
			const float ScaledDamage = Pair.Value.GetValueAtLevel(OwningAbility->GetAbilityLevel());
		
			// 할당받은 DamageEffectClass를 기반으로 GameplayEffectSpec을 생성합니다.
			FGameplayEffectSpecHandle DamageSpecHandle = ASC->MakeOutgoingSpec(EffectClass, 1.f, EffectContextHandle);
		
			// Spec 안에 SetByCallerMagnitudes라는 이름의 TMap이 있으며, 거기에 Tag를 키, Damage를 밸류로 값을 추가하는 함수입니다.
			// 이 값은 GetSetByCallerMagnitude로 꺼내올 수 있습니다.
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);

			DamageSpecs.Add(DamageSpecHandle);
		}
	}
	
	return DamageSpecs;
}

void UEffectApplier_Damage::CauseDamage(const UGameplayAbility* OwningAbility, AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& DamageSpecs)
{
	if (EffectContextHandle.IsValid())
	{
		// 대상을 관련 액터에 추가합니다.
		TArray<TWeakObjectPtr<AActor>> TargetActors;
		TargetActors.Add(TargetActor);
		EffectContextHandle.AddActors(TargetActors);
	}

	for (auto& Spec : DamageSpecs)
	{
		if (Spec.Data.IsValid())
		{
			UAbilitySystemComponent* SourceASC = OwningAbility->GetAbilitySystemComponentFromActorInfo();
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
			if (SourceASC && TargetASC)
			{
				SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
			}
		}
	}
}
