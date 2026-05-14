// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "LetheAttributeSet.generated.h"

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

/** 원문 그대로 사용하면 너무 추악하기 때문에 한 번 이쁘게 포장합니다. */
template<class T>
using TStaticFuncPtr = TBaseStaticDelegateInstance<T, FDefaultDelegateUserPolicy>::FFuncPtr;

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

UCLASS()
class LETHE_API ULetheAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	ULetheAttributeSet();

	//~ Begin UAttributeSet Interface
	/** GameplayEffect의 적용으로 인해 Attribute에 변동사항이 있으면 호출되는 함수입니다. */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	/** Attribute의 값이 변화할 때 호출되는 함수입니다. */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	//~ End UAttributeSet Interface

	static void InitializeAttributeTagMap();

private:
	/** GE 적용 시점에 Source와 Target을 편리하게 추적하기 위해 구조체에 그 정보를 채워주는 함수입니다. */
	void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& EffectProperties) const;
	
	void ApplyIncomingDamage(const FEffectProperties& Props, const FGameplayEffectModCallbackData& Data);

public:
	/** Attribute와 그에 해당하는 Tag를 매핑한 TMap입니다. */
	static TMap<FGameplayAttribute, FGameplayTag> AttributesToTags;

	/**
	 * Vital Attributes
	 */
	
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Vital Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, Health);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MaxHealth);
	
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Vital Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, Mana);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MaxMana);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData Cost;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, Cost);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxCost;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MaxCost);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MoveDistance;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MoveDistance);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxMoveDistance;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MaxMoveDistance);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData ManaRecovery;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, ManaRecovery);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData CostRecovery;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, CostRecovery);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MoveDistanceRecovery;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MoveDistanceRecovery);

	/**
	 * Stat Attributes
	 */

	/*
	UPROPERTY(BlueprintReadOnly, Category = "Stat Attributes")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, Attack);

	...
	
	*/
	
	/**
	 * Meta Attributes
	 * 
	 * 값 변화가 있을 때 위에서 선언한 Attribute에 바로 적용하는 게 아니라 중간다리 역할을 해주는 Attribute를 선언해 사용합니다.
	 * Meta Attribute는 복제되지 않습니다.
	 * 
	 * Meta Attribute 없이 직접 적용하는 방식은 각 효과의 순서가 매우 중요해집니다.
	 * 예를 들어 데미지 경감 효과를 나중에 계산한다면 캐릭터의 Health가 양수임에도 잠시 0 이하로 내려가는 상황이 발생, 캐릭터가 의도치 않게 사망할 수 있습니다.
	 * Meta Attribute를 사용하면 여러 효과(데미지 경감, 추가 피해 등)가 동시에 적용될 때 로직 순서에 덜 신경 써도 되며
	 * 중복 적용, 누락 등의 문제를 자연스럽게 방지할 수 있습니다.
	 */
	
	UPROPERTY(BlueprintReadOnly, Category = "Meta Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, IncomingDamage);
};
