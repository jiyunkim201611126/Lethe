// Fill out your copyright notice in the Description page of Project Settings.

#include "LetheAttributeSet.h"
#include "Lethe/Manager/LetheGameplayTags.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "LetheAbilitySystemLibrary.h"
#include "Lethe/Interface/CombatInterface.h"

TMap<FGameplayAttribute, FGameplayTag> ULetheAttributeSet::AttributesToTags;

ULetheAttributeSet::ULetheAttributeSet()
{
}

void ULetheAttributeSet::InitializeAttributeTagMap()
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (AttributesToTags.IsEmpty())
	{
		AttributesToTags.Reserve(9);

		AttributesToTags.Emplace(GetHealthAttribute(), LetheGameplayTags.Attributes_Vital_Health);
		AttributesToTags.Emplace(GetMaxHealthAttribute(), LetheGameplayTags.Attributes_Vital_MaxHealth);
		AttributesToTags.Emplace(GetManaAttribute(), LetheGameplayTags.Attributes_Vital_Mana);
		AttributesToTags.Emplace(GetMaxManaAttribute(), LetheGameplayTags.Attributes_Vital_MaxMana);
		AttributesToTags.Emplace(GetCostAttribute(), LetheGameplayTags.Attributes_Vital_Cost);
		AttributesToTags.Emplace(GetMaxCostAttribute(), LetheGameplayTags.Attributes_Vital_MaxCost);
		AttributesToTags.Emplace(GetManaRecoveryAttribute(), LetheGameplayTags.Attributes_Vital_ManaRecovery);
		AttributesToTags.Emplace(GetCostRecoveryAttribute(), LetheGameplayTags.Attributes_Vital_CostRecovery);
		AttributesToTags.Emplace(GetIncomingDamageAttribute(), LetheGameplayTags.Attributes_Meta_IncomingDamage);
	}
}

void ULetheAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// TODO: Source가 사망한 경우 return

	FEffectProperties Props;
	SetEffectProperties(Data, Props);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
	}
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		ApplyIncomingDamage(Props, Data);
	}
	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	if (Data.EvaluatedData.Attribute == GetCostAttribute())
	{
		SetCost(FMath::Clamp(GetCost(), 0.f, GetMaxCost()));
	}
}

void ULetheAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void ULetheAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& EffectProperties) const
{
	EffectProperties.EffectContextHandle = Data.EffectSpec.GetContext();
	EffectProperties.SourceASC = EffectProperties.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();

	if (IsValid(EffectProperties.SourceASC) && EffectProperties.SourceASC->AbilityActorInfo.IsValid() && EffectProperties.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		EffectProperties.SourceAvatarActor = EffectProperties.SourceASC->AbilityActorInfo->AvatarActor.Get();
		EffectProperties.SourceController = EffectProperties.SourceASC->AbilityActorInfo->PlayerController.Get();
		if (EffectProperties.SourceController == nullptr && EffectProperties.SourceAvatarActor != nullptr)
		{
			if (const APawn* Pawn = Cast<APawn>(EffectProperties.SourceAvatarActor))
			{
				EffectProperties.SourceController = Pawn->GetController();
			}
		}
		if (EffectProperties.SourceController)
		{
			EffectProperties.SourceCharacter = Cast<ACharacter>(EffectProperties.SourceController->GetPawn());
		}
	}

	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		EffectProperties.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		EffectProperties.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		EffectProperties.TargetCharacter = Cast<ACharacter>(EffectProperties.TargetAvatarActor);
		EffectProperties.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(EffectProperties.TargetAvatarActor);
	}	
}

void ULetheAttributeSet::ApplyIncomingDamage(const FEffectProperties& Props, const FGameplayEffectModCallbackData& Data)
{
	const float LocalIncomingDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	if (!Props.TargetAvatarActor || !Props.SourceAvatarActor)
	{
		return;
	}
	
	if (LocalIncomingDamage > 0.01f)
	{
		// 새로운 체력을 계산해 할당합니다.
		const float NewHealth = GetHealth() - LocalIncomingDamage;
		SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
		UE_LOG(LogTemp, Log, TEXT("SVAttributeSet - Damage Applied"));

		// 데미지 적용 시 발생하는 Source와 Target의 Attribute 변화를 계산해 가져옵니다.
		TMap<FGameplayAttribute, float> OutDataForSource;
		TMap<FGameplayAttribute, float> OutDataForTarget;
		ULetheAbilitySystemLibrary::ResolveDamageRules(Props.SourceASC, Props.TargetASC, LocalIncomingDamage, OutDataForSource, OutDataForTarget);

		// TODO: 위 함수를 통해 나온 OutData를 실제로 적용해야 합니다.
		
		const bool bFatal = NewHealth <= 0.f;
		if (bFatal)
		{
			if (ICombatInterface* Combat = Cast<ICombatInterface>(Props.TargetAvatarActor))
			{
				Combat->Die();
			}
		}
	}
	else
	{
		// 데미지가 0.01보다 작으면 체력 감소를 적용하지 않습니다.
		SetIncomingDamage(0.f);
	}

	// TODO: Damage Text 출력 등
}
