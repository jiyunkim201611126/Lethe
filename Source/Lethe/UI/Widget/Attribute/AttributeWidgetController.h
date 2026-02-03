// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "AttributeWidgetController.generated.h"

struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChanged, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedWithMaxValue, float, CurrentValue, float, MaxValue);

UCLASS(Abstract, Blueprintable)
class LETHE_API UAttributeWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	virtual void BroadcastInitialValue() override;
	//~ End LetheWidgetController Interface

	// Health Attribute에만 바인드를 수행하는 Enemy 전용 함수입니다.
	void BindCallbacksForEnemy(ULetheAbilitySystemComponent* ASC, const ULetheAttributeSet* AS);

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data);
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data);
	void OnManaChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaChanged(const FOnAttributeChangeData& Data);
	void OnCostChanged(const FOnAttributeChangeData& Data);

public:
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChangedWithMaxValue OnHealthPercentChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChangedWithMaxValue OnManaPercentChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChanged OnCostChangedDelegate;

private:
	float Health = 0.f;
	float MaxHealth = 0.f;

	// 아래는 Player Character 전용 변수입니다.
	// TODO: 구현 편의성을 위해 이렇게 했는데, 나중에 값이 너무 많아지면 상속시켜서 나눠주는 게 좋을 듯
	float Mana = 0.f;
	float MaxMana = 0.f;
	float Cost = 0.f;
};
