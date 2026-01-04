// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheWidgetController.h"
#include "OverlayWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAttributeChangedSignature, const UAbilitySystemComponent*, OwnerASC, float, NewValue);

UCLASS(Abstract, Blueprintable)
class LETHE_API UOverlayWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void BindCallbacksToDependencies() override;
	virtual void BroadcastInitialValue() override;
	//~ End LetheWidgetController Interface

public:
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChangedSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category = "GAS | Attributes")
	FOnAttributeChangedSignature OnMaxHealthChanged;
};
