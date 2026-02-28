// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeWidgetController.h"
#include "PlayerAttributeWidgetController.generated.h"

class ULetheCardAbility;

UCLASS(Abstract, Blueprintable)
class LETHE_API UPlayerAttributeWidgetController : public UAttributeWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin LetheWidgetController Interface
	virtual void BindCallbacks(ULetheAbilitySystemComponent* ASC, ULetheAttributeSet* AS) override;
	//~ End LetheWidgetController Interface

protected:
	void OnCardSelected(ULetheAbilitySystemComponent* CardOwnerASC, const ULetheCardAbility* CardAbility);

	virtual void OnOtherTileDetected(const AActor* LastActor, const AActor* CurrentActor, UAbilitySystemComponent* SourceASC, const ULetheCardAbility* CardAbility) override;
	virtual void StartAllPreview(const TMap<FGameplayTag, float>& InPreviewData) override;
	virtual void StopAllPreview() override;

private:
	void OnManaChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastManaChanged() const;
	
	void OnCostChanged(const FOnAttributeChangeData& AttributeData);
	void BroadcastCostChanged() const;
};
