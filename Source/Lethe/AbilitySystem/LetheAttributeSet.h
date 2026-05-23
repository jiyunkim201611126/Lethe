// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AttributeHelper.h"
#include "LetheAttributeSet.generated.h"

UCLASS(NotBlueprintable)
class LETHE_API ULetheAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	static void InitBaseAttributeTagMap();
	
	//~ Begin UAttributeSet Interface
	/** GameplayEffect의 적용으로 인해 Attribute에 변동사항이 있으면 호출되는 함수입니다. */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	/** Attribute의 값이 변화할 때 호출되는 함수입니다. */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;
	//~ End UAttributeSet Interface

private:
	
	void ApplyIncomingDamage(const FEffectProperties& Props, const FGameplayEffectModCallbackData& Data);

public:
	/** Attribute와 그에 해당하는 Tag를 매핑한 TMap입니다. */
	static TMap<FGameplayAttribute, FGameplayTag> BaseAttributesToTags;

	/**
	 * Vital Attributes
	 */
	
	UPROPERTY(BlueprintReadOnly, SaveGame, Category = "Vital Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, Health);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MaxHealth);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MoveRange;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MoveRange);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MaxMoveRange;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MaxMoveRange);
	
	UPROPERTY(BlueprintReadOnly, Category = "Vital Attributes")
	FGameplayAttributeData MoveRangeRecovery;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, MoveRangeRecovery);

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
	 * 
	 * Meta Attribute 없이 직접 적용하는 방식은 각 효과의 순서가 매우 중요해집니다.
	 * 예를 들어 데미지 경감 효과를 나중에 계산한다면 캐릭터의 Health가 양수임에도 잠시 0 이하로 내려가는 상황이 발생, 캐릭터가 의도치 않게 사망할 수 있습니다.
	 * 물론 잘 짜면 안 그러겠지만, '신경 써야 한다'는 부분이 문제입니다.
	 * Meta Attribute를 사용하면 여러 효과(데미지 경감, 추가 피해 등)가 동시에 적용될 때 로직 순서에 덜 신경 써도 되며
	 * 중복 적용, 누락 등의 문제를 자연스럽게 방지할 수 있습니다.
	 */
	
	UPROPERTY(BlueprintReadOnly, Category = "Meta Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(ULetheAttributeSet, IncomingDamage);
};
