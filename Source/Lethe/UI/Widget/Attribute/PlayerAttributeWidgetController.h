// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
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

private:
	// CDO를 캐싱할 멤버변수기 때문에 템플릿에도 const를 붙여줍니다.
	TWeakObjectPtr<const ULetheGameplayAbility> SelectedCardAbility;

	// 해당 WidgetController와 연관된 ASC의 카드가 선택되면, Ability의 Cost에 대한 예고를 보여주기 위해 아래에 데이터가 캐싱됩니다.
	TMap<FGameplayAttribute, float> AbilityCostPreviewData;
};
