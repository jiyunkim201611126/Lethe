// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "AttributeWidgetController.generated.h"

struct FOnAttributeChangeData;

DECLARE_MULTICAST_DELEGATE_OneParam(FOnAttributeChanged, float);

UCLASS(Abstract, Blueprintable)
class LETHE_API UAttributeWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void SetWidgetControllerParams(const FWidgetControllerParams& WidgetControllerParams) override;
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	virtual void BroadcastInitialValue() override;
	//~ End LetheWidgetController Interface

protected:
	virtual void OnCancelCardSelect();

private:
	void OnHealthChanged(const FOnAttributeChangeData& Data) const;
	void OnMaxHealthChanged(const FOnAttributeChangeData& Data) const;

public:
	FOnAttributeChanged OnHealthChangedDelegate;
	FOnAttributeChanged OnMaxHealthChangedDelegate;
	
	TMap<FGameplayTag, FOnAttributeChanged> OnPreviewDataDelegateMap;
};
