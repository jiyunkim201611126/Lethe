// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "LetheAbilitySystemLibrary.generated.h"

class ATile;
class ALetheHUD;
class UCardPanelWidgetController;
class UOverlayWidgetController;

UCLASS()
class LETHE_API ULetheAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:	
	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | WidgetController", meta = (WorldContext = "WorldContextObject"))
	static UOverlayWidgetController* GetOverlayWidgetController(const UObject* WorldContextObject);
	
	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | WidgetController", meta = (WorldContext = "WorldContextObject"))
	static UCardPanelWidgetController* GetCardPanelWidgetController(const UObject* WorldContextObject);

	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | Ability", meta = (WorldContext = "WorldContextObject"))
	static bool CanUseAbilityByActorAndFloorGap(const UObject* WorldContextObject, const AActor* SourceActor, const AActor* TargetActor, const int32 MaxFloorGap);

	UFUNCTION(BlueprintPure, Category = "LetheAbilitySystemLibrary | Ability", meta = (WorldContext = "WorldContextObject"))
	static bool CanUseAbilityByTileAndFloorGap(const UObject* WorldContextObject, const ATile* SourceTile, const ATile* TargetTile, const int32 MaxFloorGap);
};
