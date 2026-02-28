// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectApplier_Damage.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Lethe/Manager/LetheTextManager.h"

void UEffectApplier_Damage::ApplyEffect(UGameplayAbility* OwningAbility, AActor* TargetActor)
{
	TArray<FGameplayEffectSpecHandle> OutSpecHandles;
	if (TryMakeSpecHandlesWithContextHandle(OwningAbility, OutSpecHandles))
	{
		CauseDamage(OwningAbility, TargetActor, OutSpecHandles);
	}
}

bool UEffectApplier_Damage::TryMakeSpecHandles(const UAbilitySystemComponent* SourceASC, const UGameplayAbility* OwningAbility, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles, const bool bIsPreview) const
{
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		if (Pair.Value.IsValid())
		{
			const float ScaledDamage = Pair.Value.GetValueAtLevel(OwningAbility->GetAbilityLevel());
		
			// 할당받은 DamageEffectClass를 기반으로 GameplayEffectSpec을 생성합니다.
			FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, InContextHandle);
		
			// Spec 안에 SetByCallerMagnitudes라는 이름의 TMap이 있으며, 거기에 Tag를 키, Damage를 밸류로 값을 추가하는 함수입니다.
			// 이 값은 GetSetByCallerMagnitude로 꺼내올 수 있습니다.
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);

			OutSpecHandles.Emplace(DamageSpecHandle);
		}
	}
	return !OutSpecHandles.IsEmpty();
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

FText UEffectApplier_Damage::GetDescriptionText(const int32 InLevel) const
{
	TArray<FText> DamageTexts;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		DamageTexts.Emplace(GetDamageText(InLevel, Pair.Key));
	}

	return FText::Join(FText::FromString(TEXT(" ")), DamageTexts);
}

FText UEffectApplier_Damage::GetDamageText(const int32 InLevel, const FGameplayTag& InDamageTag) const
{
	const FScalableFloat* DamageValue = DamageValues.Find(InDamageTag);
	check(DamageValue);
	
	const float ScaledDamage = DamageValue->GetValueAtLevel(InLevel);
	FText DamageText = FLetheTextManager::GetText(EStringTableType::Card, InDamageTag.ToString(), ScaledDamage);
	return DamageText;
}
