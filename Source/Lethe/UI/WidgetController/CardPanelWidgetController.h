// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "LetheWidgetController.h"
#include "CardPanelWidgetController.generated.h"

class ULetheGameplayAbility;
class UCardViewData;
struct FGameplayTag;
struct FCardViewInfo;

DECLARE_DELEGATE_TwoParams(FOnAbilityUpdatedSignature, ULetheAbilitySystemComponent*, const FCardViewInfo*)

UCLASS(Abstract, Blueprintable)
class LETHE_API UCardPanelWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	//~ Begin ULetheWidgetController Interface
	virtual void BindCallbacksToDependencies() override;
	//~ End of ULetheWidgetController Interface

	FVector2D GetCardSize() const;

private:
	void OnGiveAbility(ULetheAbilitySystemComponent* OwnerASC, ULetheGameplayAbility* InAbility) const;

public:
	FOnAbilityUpdatedSignature OnAbilityUpdatedDelegate;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Card")
	TObjectPtr<UCardViewData> CardViewData;
};
