// Copyright JETBLU, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HighlightInterface.generated.h"

UINTERFACE()
class UHighlightInterface : public UInterface
{
	GENERATED_BODY()
};

class LETHE_API IHighlightInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HighlightActorByMouse();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UnhighlightActorByMouse();
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void HighlightActorByCard(const int32 InOutlineColor);
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void UnhighlightActorByCard();
};
