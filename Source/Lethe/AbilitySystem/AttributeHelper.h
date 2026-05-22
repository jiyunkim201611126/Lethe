// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameFramework/Character.h"
#include "AttributeHelper.generated.h"

/**
 * Getter, Setter, Initter를 자동 생성해주는 매크로를 호출하기 위해 정의하는 구문입니다.
 * 이 구문이 없으면 Getter, Setter, Initter를 변수마다 모두 작성해야 합니다.
 * 즉 Boilerplate 코드를 줄여주는 구문입니다.
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * Attribute에게 변화가 적용되는 모든 상황에 대해서 Source와 Target을 추적하기 위해 선언, 초기화하는 구조체입니다.
 * 사실상 편의성을 위해 사용하는 구조체이며, 멤버 변수로 사용하지 않기 때문에 내부는 모두 일반 Raw 포인터로 선언되어 있습니다.
 */
USTRUCT(BlueprintType)
struct FEffectProperties
{
	GENERATED_BODY()

	FEffectProperties(){}

	FGameplayEffectContextHandle EffectContextHandle;

	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;

	UPROPERTY()
	AActor* SourceAvatarActor = nullptr;

	UPROPERTY()
	AController* SourceController = nullptr;

	UPROPERTY()
	ACharacter* SourceCharacter = nullptr;

	UPROPERTY()
	UAbilitySystemComponent* TargetASC = nullptr;

	UPROPERTY()
	AActor* TargetAvatarActor = nullptr;

	UPROPERTY()
	AController* TargetController = nullptr;

	UPROPERTY()
	ACharacter* TargetCharacter = nullptr;
};

inline void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& EffectProperties)
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
