// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LetheAbilitySystemLibrary.generated.h"

class ALetheHUD;
class UOverlayWidgetController;
class UCardPanelWidgetController;

UCLASS()
class LETHE_API ULetheAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:	
	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | WidgetController", meta = (WorldContext = "WorldContextObject"))
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | WidgetController", meta = (WorldContext = "WorldContextObject"))
	static UCardPanelWidgetController* GetCardPanelWidgetController(const UObject* WorldContextObject);
};
