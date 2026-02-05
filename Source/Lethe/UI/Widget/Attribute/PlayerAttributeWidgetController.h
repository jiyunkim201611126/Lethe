// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeWidgetController.h"
#include "GameplayTagContainer.h" //CB_ADDED 260205
#include "PlayerAttributeWidgetController.generated.h"

struct FGameplayTag;
class ULetheGameplayAbility;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPreviewValueChanged, const float);

UCLASS(Abstract, Blueprintable)
class LETHE_API UPlayerAttributeWidgetController : public UAttributeWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	virtual void BroadcastInitialValue() override;
	//~ End LetheWidgetController Interface

private:
	void OnManaChanged(const FOnAttributeChangeData& Data);
	void OnMaxManaChanged(const FOnAttributeChangeData& Data);
	void OnCostChanged(const FOnAttributeChangeData& Data);

	void OnCardSelected(const ULetheAbilitySystemComponent* CardOwnerASC, const ULetheGameplayAbility* CardAbility);

public:
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChangedWithMaxValue OnManaChangedDelegate;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChanged OnCostChangedDelegate;

	TMap<FGameplayTag, FOnPreviewValueChanged> OnPreviewDataChangedMap;

private:
	float Mana = 0.f;
	float MaxMana = 0.f;
	float Cost = 0.f;

	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheGameplayAbility> SelectedCardAbility;

	// 해당 WidgetController와 연관된 ASC의 카드가 선택되면, Ability의 Cost에 대한 예고를 보여주기 위해 아래에 데이터가 캐싱됩니다.
	TMap<FGameplayTag, float> AbilityCostPreviewData;
};
