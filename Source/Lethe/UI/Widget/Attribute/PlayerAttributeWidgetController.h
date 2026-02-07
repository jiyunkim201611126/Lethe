// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeWidgetController.h"
#include "PlayerAttributeWidgetController.generated.h"

class ULetheGameplayAbility;

UCLASS(Abstract, Blueprintable)
class LETHE_API UPlayerAttributeWidgetController : public UAttributeWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	//~ End LetheWidgetController Interface

protected:
	void OnCardSelected(const ULetheAbilitySystemComponent* CardOwnerASC, const ULetheGameplayAbility* CardAbility);
	void OnCancelCardSelect();

	virtual void StartAllPreview() override;
	virtual void StopAllPreview() override;

private:
	void OnManaChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastManaChanged() const;
	
	void OnCostChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastCostChanged() const;

private:	
	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheGameplayAbility> SelectedCardAbility;
};
