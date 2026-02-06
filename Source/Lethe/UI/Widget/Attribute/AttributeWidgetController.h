// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Lethe/UI/Widget/LetheWidgetController.h"
#include "AttributeWidgetController.generated.h"

struct FGameplayAttribute;
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
	//~ End LetheWidgetController Interface

protected:	
	void OnAttributeChanged(const FOnAttributeChangeData& AttributeData);
	void OnAttributePreviewChanged(const FGameplayAttribute& Attribute, const float PreviewDeltaValue);

public:
	TMap<FGameplayTag, FOnAttributeChanged> OnAttributeChangedDelegates;
	TMap<FGameplayTag, FOnAttributeChanged> OnAttributePreviewChangedDelegates;
};
