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
	// AttributeTag를 Key로, 델리게이트를 Value로 매핑하는 TMap입니다.
	// 기존 Attribute 하나당 하나의 델리게이트를 선언하는 방식은 Attribute가 많아질 경우 가독성이 저하되므로, 이와 같은 방식을 사용합니다.
	TMap<FGameplayTag, FOnAttributeChanged> OnAttributeChangedDelegates;
	TMap<FGameplayTag, FOnAttributeChanged> OnAttributePreviewChangedDelegates;
};
