// Copyright JETBLU, Inc. All Rights Reserved.

#include "EffectSpecBuilder_DamageWithConsumeAllCost.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Lethe/AbilitySystem/PlayerAttributeSet.h"
#include "Lethe/Interface/PlayerCharacterInterface.h"
#include "Lethe/Manager/LetheTextManager.h"

bool FEffectSpecBuilder_DamageWithConsumeAllCost::TryBuildSourceEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	OutSpecHandles.Reset();

	if (!SourceASC)
	{
		return false;
	}

	const AActor* AvatarActor = SourceASC->GetAvatarActor();
	if (!AvatarActor)
	{
		return false;
	}

	// Cost 개념은 플레이어 캐릭터에게만 존재합니다.
	if (AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		const int32 CurrentCost = static_cast<int32>(SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute()));
		OutSpecHandles.Reserve(CurrentCost);

		// Cost 수만큼 반복합니다.
		for (int32 Index = 0; Index < CurrentCost; ++Index)
		{
			// Cost를 1 소모하는 EffectSpec을 만들어 추가합니다.
			const FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(CostEffectClass, 1.f, InContextHandle);
			OutSpecHandles.Add(SpecHandle);
		}
	}

	return !OutSpecHandles.IsEmpty();
}

bool FEffectSpecBuilder_DamageWithConsumeAllCost::TryBuildTargetEffectSpecs(UAbilitySystemComponent* SourceASC, const FGameplayEffectContextHandle& InContextHandle, TArray<FGameplayEffectSpecHandle>& OutSpecHandles) const
{
	OutSpecHandles.Reset();

	if (!SourceASC)
	{
		return false;
	}

	const AActor* AvatarActor = SourceASC->GetAvatarActor();
	if (!AvatarActor)
	{
		return false;
	}

	if (AvatarActor->Implements<UPlayerCharacterInterface>())
	{
		const int32 CurrentCost = static_cast<int32>(SourceASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute()));
		OutSpecHandles.Reserve(CurrentCost * DamageValues.Num());

		// Cost 수만큼 반복합니다.
		for (int32 Index = 0; Index < CurrentCost; ++Index)
		{
			// 지정된 Damage를 주는 EffectSpec을 만들어 추가합니다.
			for (const auto& Pair : DamageValues)
			{
				if (Pair.Value.IsValid())
				{
					const float ScaledDamage = Pair.Value.GetValueAtLevel(InContextHandle.GetAbilityLevel());
					FGameplayEffectSpecHandle DamageSpecHandle = SourceASC->MakeOutgoingSpec(EffectClass, 1.f, InContextHandle);
					UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(DamageSpecHandle, Pair.Key, ScaledDamage);
					OutSpecHandles.Add(DamageSpecHandle);
				}
			}
		}
	}

	return !OutSpecHandles.IsEmpty();
}

void FEffectSpecBuilder_DamageWithConsumeAllCost::GetDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel, FText& OutDescription) const
{
	const FString UseAllCostAndChainLocalKey = TEXT("UseAllCostAndChain");
	const FText UseAllCostAndChainText = FLetheTextManager::GetText(EStringTableType::CardDescription, UseAllCostAndChainLocalKey);
	
	FText DamageDescriptionText;
	Super::GetDescription(OwnerASC, InLevel, DamageDescriptionText);
	
	OutDescription = FText::Format(INVTEXT("{0} {1}"), UseAllCostAndChainText, DamageDescriptionText);
}

int32 FEffectSpecBuilder_DamageWithConsumeAllCost::GetValueForDescription(const UAbilitySystemComponent* OwnerASC, const int32 InLevel) const
{
	float AllDamage = 0.f;
	for (const TPair<FGameplayTag, FScalableFloat>& Pair : DamageValues)
	{
		AllDamage += Pair.Value.GetValueAtLevel(InLevel);
	}
	// 설명에 텍스트를 표시하기 위한 값을 가져가는 상황이기 때문에 단순하게 데미지를 Cost와 곱해서 반환합니다.
	return AllDamage * (OwnerASC ? OwnerASC->GetNumericAttribute(UPlayerAttributeSet::GetCostAttribute()) : 1.f);
}
