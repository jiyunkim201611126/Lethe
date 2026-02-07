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

	virtual void StartAllPreview() override;
	virtual void StopAllPreview() override;

private:
	void OnManaChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastManaChanged() const;
	
	void OnCostChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastCostChanged() const;

private:
	// 카드가 선택되면 해당하는 Ability의 Cost가 적용되는 경우 어떤 Attribute 변화가 있는지를 캐싱해두는 TMap입니다.
	TMap<FGameplayAttribute, float> CachedAbilityCostPreviewData;
};
