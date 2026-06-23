// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectApplier_Damage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"

UEffectApplier_Damage::UEffectApplier_Damage()
{
	EffectApplierTag = FGameplayTag::RequestGameplayTag(FName(TEXT("EffectApplier.Damage.Default")));
}

void UEffectApplier_Damage::ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor)
{
	FGameplayEffectContextHandle EffectContextHandle;
	MakeEffectContextHandle(OwningAbility, EffectContextHandle);

	TArray<TWeakObjectPtr<AActor>> TargetActors;
	TargetActors.Add(TargetActor);
	EffectContextHandle.AddActors(TargetActors);
	
	TArray<FGameplayEffectSpecHandle> OutSpecHandles;
	if (TryMakeSpecHandlesWithContextHandle(OwningAbility, EffectContextHandle, OutSpecHandles))
	{
		CauseDamage(OwningAbility, TargetActor, OutSpecHandles);
	}
}

bool UEffectApplier_Damage::TryPrepareSpecHandles(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bPreview) const
{
	if (!SourceASC)
	{
		return false;
	}
	
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		if (Pair.Value.IsValid())
		{
			const float ScaledDamage = Pair.Value.GetValueAtLevel(InContextHandle.GetAbilityLevel());
		
			// 할당받은 DamageEffectClass를 기반으로 GameplayEffectSpec을 생성합니다.
			FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, InContextHandle);
		
			// Spec 안에 SetByCallerMagnitudes라는 이름의 TMap이 있으며, 거기에 Tag를 키, Damage를 밸류로 값을 추가하는 함수입니다.
			// 이 값은 GetSetByCallerMagnitude로 꺼내올 수 있습니다.
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);

			OutSpecHandles.Add(DamageSpecHandle);
		}
	}
	return !OutSpecHandles.IsEmpty();
}

void UEffectApplier_Damage::CauseDamage(const UGameplayAbility* OwningAbility, AActor* TargetActor, const TArray<FGameplayEffectSpecHandle>& DamageSpecs) const
{
	if (!TargetActor)
	{
		return;
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

int32 UEffectApplier_Damage::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	float AllDamage = 0.f;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		AllDamage += Pair.Value.GetValueAtLevel(InLevel);
	}
	return AllDamage;
}
