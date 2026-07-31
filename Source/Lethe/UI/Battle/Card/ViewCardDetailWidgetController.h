// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Lethe/UI/Framework/LetheWidgetController.h"
#include "ViewCardDetailWidgetController.generated.h"

class ALethePlayerController;
struct FGameplayAbilitySpecHandle;

UCLASS(Abstract, Blueprintable)
class LETHE_API UViewCardDetailWidgetController : public ULetheWidgetController
{
	GENERATED_BODY()

public:
	void GetCardDescriptionText(const ULetheAbilitySystemComponent* OwnerASC, const FGameplayAbilitySpecHandle AbilitySpecHandle, FText& OutDescription) const;

private:
	UPROPERTY()
	TObjectPtr<ALethePlayerController> LethePlayerController;
};
