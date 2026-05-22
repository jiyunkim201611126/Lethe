// Copyright JETBLU, Inc. All Rights Reserved.

#include "LetheAttributeSet.h"
#include "GameplayEffectExtension.h"
#include "LetheAbilitySystemLibrary.h"
#include "Lethe/LetheLog.h"
#include "Lethe/Interface/CombatInterface.h"
#include "Lethe/Manager/LetheGameplayTags.h"

TMap<FGameplayAttribute, FGameplayTag> ULetheAttributeSet::BaseAttributesToTags;

void ULetheAttributeSet::InitBaseAttributeTagMap()
{
	const FLetheGameplayTags& LetheGameplayTags = FLetheGameplayTags::Get();
	if (BaseAttributesToTags.IsEmpty())
	{
		BaseAttributesToTags.Reserve(6);

		BaseAttributesToTags.Add(GetHealthAttribute(), LetheGameplayTags.Attribute_Vital_Health);
		BaseAttributesToTags.Add(GetMaxHealthAttribute(), LetheGameplayTags.Attribute_Vital_MaxHealth);
		BaseAttributesToTags.Add(GetMoveDistanceAttribute(), LetheGameplayTags.Attribute_Vital_MoveDistance);
		BaseAttributesToTags.Add(GetMaxMoveDistanceAttribute(), LetheGameplayTags.Attribute_Vital_MaxMoveDistance);
		BaseAttributesToTags.Add(GetMoveDistanceRecoveryAttribute(), LetheGameplayTags.Attribute_Vital_MoveDistanceRecovery);
		BaseAttributesToTags.Add(GetIncomingDamageAttribute(), LetheGameplayTags.Attribute_Meta_IncomingDamage);
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
	if (Data.EvaluatedData.Attribute == GetMoveDistanceAttribute())
	{
		SetMoveDistance(FMath::Clamp(GetMoveDistance(), 0.f , GetMaxMoveDistance()));
	}
}

void ULetheAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);
}

void ULetheAttributeSet::ApplyIncomingDamage(const FEffectProperties& Props, const FGameplayEffectModCallbackData& Data)
{
	const float LocalIncomingDamage = GetIncomingDamage();
	SetIncomingDamage(0.f);

	if (!Props.TargetAvatarActor || !Props.SourceAvatarActor)
	{
		return;
	}
	
	// 데미지가 0.01보다 작으면 체력 감소를 적용하지 않습니다.
	if (LocalIncomingDamage > 0.01f)
	{
		// 새로운 체력을 계산해 할당합니다.
		const float NewHealth = GetHealth() - LocalIncomingDamage;
		SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
		
		LETHE_LOG(LogAttrbitueSet, Log, "SVAttributeSet - Damage Applied");

		// 데미지 적용 시 발생하는 Source와 Target의 Attribute 변화를 계산해 가져옵니다.
		TMap<FGameplayAttribute, float> OutDataForSource;
		TMap<FGameplayAttribute, float> OutDataForTarget;
		ULetheAbilitySystemLibrary::ResolveDamageRules(Props.SourceASC, Props.TargetASC, LocalIncomingDamage, OutDataForSource, OutDataForTarget);

		// TODO: 위 함수를 통해 나온 OutData를 실제로 적용해야 합니다.
		
		const bool bFatal = NewHealth <= 0.f;
		if (ICombatInterface* Combat = Cast<ICombatInterface>(Props.TargetAvatarActor))
		{
			bFatal ? Combat->Die() : Combat->OnDamageTaken();
		}
	}

	// TODO: Damage Text 출력 등
}
